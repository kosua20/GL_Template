#include "VarianceShadowMapArray.hpp"
#include "scene/Scene.hpp"
#include "graphics/GLUtilities.hpp"

VarianceShadowMap2DArray::VarianceShadowMap2DArray(const std::vector<std::shared_ptr<Light>> & lights, const glm::vec2 & resolution){
	_lights = lights;
	const Descriptor descriptor = {Layout::RG32F, Filter::LINEAR, Wrap::CLAMP};
	_map = std::unique_ptr<Framebuffer>(new Framebuffer(TextureShape::Array2D, uint(resolution.x), uint(resolution.y), uint(lights.size()), 1, {descriptor}, true, "Shadow map 2D array"));
	_blur = std::unique_ptr<BoxBlur>(new BoxBlur(false, "Shadow maps 2D"));
	_program = Resources::manager().getProgram("object_depth", "light_shadow_vertex", "light_shadow_variance");
	for(size_t lid = 0; lid < _lights.size(); ++lid){
		_lights[lid]->registerShadowMap(_map->texture(), lid);
	}
}

void VarianceShadowMap2DArray::draw(const Scene & scene) const {

	GLUtilities::setDepthState(true, TestFunction::LESS, true);
	GLUtilities::setBlendState(false);
	GLUtilities::setCullState(true, Faces::BACK);

	_map->setViewport();
	_program->use();

	for(size_t lid = 0; lid < _lights.size(); ++lid){
		const auto & light = _lights[lid];
		if(!light->castsShadow()){
			continue;
		}
		_map->bind(lid);
		GLUtilities::clearColorAndDepth(glm::vec4(1.0f), 1.0f);

		const Frustum lightFrustum(light->vp());

		for(auto & object : scene.objects) {
			if(!object.castsShadow()) {
				continue;
			}
			// Frustum culling.
			if(!lightFrustum.intersects(object.boundingBox())){
				continue;
			}
			GLUtilities::setCullState(!object.twoSided(), Faces::BACK);

			_program->uniform("hasMask", object.masked());
			if(object.masked()) {
				GLUtilities::bindTexture(object.textures()[0], 0);
			}
			const glm::mat4 lightMVP = light->vp() * object.model();
			_program->uniform("mvp", lightMVP);
			GLUtilities::drawMesh(*(object.mesh()));
		}
	}
	
	// Apply box blur.
	_blur->process(_map->texture(), *_map);
}

VarianceShadowMapCubeArray::VarianceShadowMapCubeArray(const std::vector<std::shared_ptr<PointLight>> & lights, int side){
	_lights = lights;
	const Descriptor descriptor = {Layout::RG16F, Filter::LINEAR, Wrap::CLAMP};
	_map = std::unique_ptr<Framebuffer>(new Framebuffer( TextureShape::ArrayCube, side, side, uint(lights.size()), 1,  {descriptor}, true, "Shadow map cube array"));
	_blur = std::unique_ptr<BoxBlur>(new BoxBlur(true, "Shadow maps cube"));
	_program = Resources::manager().getProgram("object_cube_depth", "light_shadow_linear_vertex", "light_shadow_linear_variance");
	for(size_t lid = 0; lid < _lights.size(); ++lid){
		_lights[lid]->registerShadowMap(_map->texture(), lid);
	}
}

void VarianceShadowMapCubeArray::draw(const Scene & scene) const {

	GLUtilities::setDepthState(true, TestFunction::LESS, true);
	GLUtilities::setCullState(true, Faces::BACK);
	GLUtilities::setBlendState(false);
	_map->setViewport();
	_program->use();

	for(size_t lid = 0; lid < _lights.size(); ++lid){
		const auto & light = _lights[lid];
		if(!light->castsShadow()){
			continue;
		}
		// Udpate the light vp matrices.
		const auto & faces = light->vpFaces();

		// Pass the world space light position, and the projection matrix far plane.
		_program->uniform("lightPositionWorld", light->position());
		_program->uniform("lightFarPlane", light->farPlane());
		for(int i = 0; i < 6; ++i){
			// We render each face sequentially, culling objects that are not visible.
			_map->bind(lid * 6 + i);
			GLUtilities::clearColorAndDepth(glm::vec4(1.0f), 1.0f);
			const Frustum lightFrustum(faces[i]);

			for(auto & object : scene.objects) {
				if(!object.castsShadow()) {
					continue;
				}
				// Frustum culling.
				if(!lightFrustum.intersects(object.boundingBox())){
					continue;
				}

				GLUtilities::setCullState(!object.twoSided(), Faces::BACK);
				const glm::mat4 mvp = faces[i] * object.model();
				_program->uniform("mvp", mvp);
				_program->uniform("m", object.model());
				_program->uniform("hasMask", object.masked());
				if(object.masked()) {
					GLUtilities::bindTexture(object.textures()[0], 0);
				}
				GLUtilities::drawMesh(*(object.mesh()));
			}
		}
	}
	// Apply box blur.
	_blur->process(_map->texture(), *_map);
}
