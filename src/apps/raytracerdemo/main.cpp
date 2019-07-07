#include "Common.hpp"
#include "Config.hpp"

#include "raycaster/Raycaster.hpp"

#include "input/Input.hpp"
#include "input/InputCallbacks.hpp"
#include "input/ControllableCamera.hpp"

#include "scene/Scene.hpp"
#include "resources/ResourcesManager.hpp"
#include "graphics/ScreenQuad.hpp"

#include "helpers/Random.hpp"
#include "helpers/System.hpp"

#include <thread>

/**
 \defgroup RaytracerDemo Raytracer demo
 \brief A basic ray tracing demo.
 \ingroup Applications
 */

class RaytracerConfig : public Config {
public:
	
	RaytracerConfig(const std::vector<std::string> & argv) : Config(argv) {
		
		// Process arguments.
		for(const auto & arg : _rawArguments){
			const std::string key = arg.key;
			const std::vector<std::string> & values = arg.values;
			
			if(key == "samples" && values.size() >= 1){
				samples = size_t(std::stoi(values[0]));
			} else if(key == "depth" && values.size() >= 1){
				depth = size_t(std::stoi(values[0]));
			} else if(key == "scene" && values.size() >= 1){
				scene = values[0];
			} else if(key == "output" && values.size() >= 1){
				outputPath = values[0];
			}  else if(key == "wxh" && values.size() >= 2){
				size[0] = std::stoi(values[0]);
				size[1] = std::stoi(values[1]);
			}
		}
		// If no path passed, setup a default one.
		if(outputPath.empty()){
			outputPath 	= "./test_" + scene + "_" + std::to_string(samples) + "_" + std::to_string(depth)
						+ "_" + std::to_string(size.x) + "x" + std::to_string(size.y) + ".png";
		}
		
		// Detail help.
		_infos.emplace_back("", "", "Raytracer");
		_infos.emplace_back("wxh", "", "Dimensions of the image.", std::vector<std::string>{"width", "height"});
		_infos.emplace_back("samples", "", "Number of samples per pixel.", "int");
		_infos.emplace_back("depth", "", "Maximum path depth.", "int");
		_infos.emplace_back("scene", "", "Name of the scene to load.", "string");
		_infos.emplace_back("output", "", "Path for the output image.", "path");
		
	}
	
	glm::ivec2 size = glm::ivec2(1024);
	size_t samples = 8; ///< Number of samples per pixel.
	size_t depth = 5; ///< Max depth of a path.
	std::string outputPath = ""; ///< Output image path.
	std::string scene = ""; ///< Scene name.
	
};

/**
 The main function of the demo.
 \param argc the number of input arguments.
 \param argv a pointer to the raw input arguments.
 \return a general error code.
 \ingroup RaytracerDemo
 */
int main(int argc, char** argv) {
	
	RaytracerConfig config(std::vector<std::string>(argv, argv+argc));
	if(config.showHelp()){
		return 0;
	}
	
	if(config.scene.empty()){
		Log::Error() << "Missing scene name." << std::endl;
		return 1;
	}
	
	// Initialize random generator;
	Random::seed();
	Resources::manager().addResources("../../../resources/pbrdemo");
	Resources::manager().addResources("../../../resources/additional");
	
	// Load geometry and create raycaster.
	Scene scene(config.scene);
	scene.init(Storage::CPU);
	
	Raycaster raycaster;
	for(const auto & obj : scene.objects){
		raycaster.addMesh(obj.mesh()->geometry, obj.model());
	}
	raycaster.updateHierarchy();
	
	// Result image.
	Image render(config.size.x, config.size.y, 3);
	const size_t samples = config.samples;
	const size_t depth = config.depth;
	const float ratio = float(config.size.x)/float(config.size.y);
	
	// Setup camera.
	Camera camera;
	camera.pose(glm::vec3(0.0f, 1.0f, 2.5f), glm::vec3(0.0f, 1.0f, 1.5f), glm::vec3(0.0f, 1.0f, 0.0f));
	camera.projection(ratio, 1.3f, 0.01f, 100.0f);
	// Compute incremental pixel shifts.
	glm::vec3 corner, dx, dy;
	camera.pixelShifts(corner, dx, dy);
	
	// Start chrono.
	auto start = std::chrono::steady_clock::now();
	
	// Parallelize on each row of the image.
	System::forParallel(0, render.height, [&](size_t y){
		for(size_t x = 0; x < render.width; ++x){
			
			for(size_t sid = 0; sid < samples; ++sid){
				// Draw random shift in [0.0,1.0f) for jittering.
				const float jx = Random::Float();
				const float jy = Random::Float();
				// Derive a position on the image plane from the pixel.
				const glm::vec2 ndcPos = glm::vec2(x + jx, y + jy) / glm::vec2(render.width, render.height);
				// Place the point on the near plane in clip space.
				const glm::vec3 worldPos = corner + ndcPos.x * dx + ndcPos.y * dy;
				
				glm::vec3 rayPos = camera.position();
				glm::vec3 rayDir = glm::normalize(worldPos - camera.position());
				
				glm::vec3 sampleColor(0.0f);
				glm::vec3 attenColor(1.0f);
				
				for(size_t did = 0; did < depth; ++did){
					// Query closest intersection.
					const Raycaster::RayHit hit = raycaster.intersects(rayPos, rayDir);
					
					// If no hit, background.
					if(!hit.hit){
						// For now the background is not emissive, it is only sampled for direct paths.
						/// \todo Support environment map fetch.
						if(depth == 0){
							const Scene::Background mode = scene.backgroundMode;
							if (mode == Scene::Background::IMAGE){
								const Image & image = scene.background.textures()[0]->images[0];
								sampleColor = image.rgbl(ndcPos.x, ndcPos.y);
							} else {
								sampleColor = scene.backgroundColor;
							}
						}
						break;
					}
					
					// Fetch geometry infos...
					const Mesh & mesh = scene.objects[hit.meshId].mesh()->geometry;
					const glm::vec3 p = rayPos + hit.dist * rayDir;
					const glm::vec3 n = Raycaster::interpolateNormal(hit, mesh);
					const glm::vec2 uv = Raycaster::interpolateUV(hit, mesh);
					
					// Compute lighting.
					// Check light visibility.
					glm::vec3 illumination(0.0f);
					for(const auto light : scene.lights){
						glm::vec3 direction;
						float attenuation;
						if(light->visible(p, raycaster, direction, attenuation)){
							const float diffuse = glm::max(glm::dot(n, direction), 0.0f);
							illumination += attenuation * diffuse * light->intensity();
						}
					}
				
					// Fetch base color from texture.
					const Image & image = scene.objects[hit.meshId].textures()[0]->images[0];
					const glm::vec3 baseColor = glm::pow(image.rgbl(uv.x, uv.y), glm::vec3(2.2f));
					
					// Bounce decay.
					attenColor *= baseColor;
					sampleColor += attenColor * illumination;
					
					// Update position and ray direction.
					if(did < depth-1){
						rayPos = p;
						// For the direction, we want to sample the hemisphere, weighted by the cosine weight to better use our samples.
						// We use the trick described by Peter Shirley in 'Raytracing in One Week-End':
						// Uniformly sample a sphere tangent to the surface, add this to the normal.
						rayDir = glm::normalize(n + Random::sampleSphere());
					}
				}
				// Modulate and store.
				render.rgb(x,y) += sampleColor;
			}
		}
	});
	
	
	// Normalize and gamma correction.
	System::forParallel(0, render.height, [&render, &samples](size_t y){
		for(size_t x = 0; x < render.width; ++x){
			const glm::vec3 color = render.rgb(x,y) / float(samples);
			render.rgb(x,y) = glm::pow(color, glm::vec3(1.0f/2.2f));
		}
	});
	
	
	// Display duration.
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
	Log::Info() << "Generation took " << duration.count() << " ms at " << render.width << "x" << render.height << "." << std::endl;
	
	// Save image.
	ImageUtilities::saveLDRImage(config.outputPath, render, false);
	return 0;
}


