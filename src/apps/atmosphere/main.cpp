#include "input/Input.hpp"
#include "input/ControllableCamera.hpp"
#include "resources/ResourcesManager.hpp"
#include "graphics/ScreenQuad.hpp"
#include "graphics/Framebuffer.hpp"
#include "graphics/GLUtilities.hpp"
#include "system/System.hpp"
#include "system/Random.hpp"
#include "system/Config.hpp"
#include "Common.hpp"

/**
 \defgroup AtmosphericScattering Atmospheric scattering
 \brief Demonstrate real-time approximate atmospheric scattering simulation.
 \see GLSL::Frag::Atmosphere
 \ingroup Applications
 */

/**
 The main function of the atmospheric scattering demo.
 \param argc the number of input arguments.
 \param argv a pointer to the raw input arguments.
 \return a general error code.
 \ingroup AtmosphericScattering
 */
int main(int argc, char ** argv) {

	// First, init/parse/load configuration.
	RenderingConfig config(std::vector<std::string>(argv, argv + argc));
	if(config.showHelp()) {
		return 0;
	}

	GLFWwindow * window = System::initWindow("Atmosphere", config);
	if(!window) {
		return -1;
	}

	Resources::manager().addResources("../../../resources/common");
	Resources::manager().addResources("../../../resources/atmosphere");

	// Seed random generator.
	Random::seed();

	glEnable(GL_DEPTH_TEST);

	// Setup the timer.
	double timer		 = glfwGetTime();
	double fullTime		 = 0.0;
	double remainingTime = 0.0;
	const double dt		 = 1.0 / 120.0; // Small physics timestep.

	// Camera.
	ControllableCamera camera;
	camera.projection(config.screenResolution[0] / config.screenResolution[1], 1.34f, 0.1f, 100.0f);
	const glm::vec2 renderResolution = float(config.internalVerticalResolution) / config.screenResolution[1] * config.screenResolution;

	// Framebuffer to store the rendered atmosphere result before tonemapping and upscaling to the window size.
	std::unique_ptr<Framebuffer> atmosphereFramebuffer(new Framebuffer(uint(renderResolution[0]), uint(renderResolution[1]), Layout::RGB32F, false));
	const Texture * precomputedScattering = Resources::manager().getTexture("scattering-precomputed", {Layout::RGB32F, Filter::LINEAR_LINEAR, Wrap::CLAMP}, Storage::GPU);

	// Atmosphere screen quad.
	const Program * atmosphereProgram = Resources::manager().getProgram2D("atmosphere");

	// Final tonemapping screen quad.
	const Program * tonemapProgram = Resources::manager().getProgram2D("tonemap");

	// Sun direction.
	glm::vec3 lightDirection(0.437f, 0.082f, -0.896f);
	lightDirection = glm::normalize(lightDirection);

	// Timing.
	const unsigned int frameCount = 20;
	std::vector<double> timings(frameCount, 0.0);
	unsigned int currentTiming = 0;
	double smoothedFrameTime   = 0.0;

	// Start the display/interaction loop.
	while(!glfwWindowShouldClose(window)) {
		// Update events (inputs,...).
		Input::manager().update();
		// Handle quitting.
		if(Input::manager().pressed(Input::KeyEscape)) {
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
		// Start a new frame for the interface.
		System::Gui::beginFrame();
		// Reload resources.
		if(Input::manager().triggered(Input::KeyP)) {
			Resources::manager().reload();
		}

		// Compute the time elapsed since last frame
		const double currentTime = glfwGetTime();
		double frameTime		 = currentTime - timer;
		timer					 = currentTime;
		camera.update();

		// Timing.
		smoothedFrameTime -= timings[currentTiming];
		timings[currentTiming] = 1000.0 * frameTime;
		smoothedFrameTime += timings[currentTiming];
		currentTiming	= (currentTiming + 1) % frameCount;
		const double sft = smoothedFrameTime / frameCount;
		ImGui::Text("%2.2f ms (%2.0f fps)", sft, 1.0 / sft * 1000.0);

		// Physics simulation
		// First avoid super high frametime by clamping.
		if(frameTime > 0.2) {
			frameTime = 0.2;
		}
		// Accumulate new frame time.
		remainingTime += frameTime;
		// Instead of bounding at dt, we lower our requirement (1 order of magnitude).
		while(remainingTime > 0.2 * dt) {
			double deltaTime = std::min(remainingTime, dt);
			// Update physics and camera.
			camera.physics(deltaTime);
			// Update timers.
			fullTime += deltaTime;
			remainingTime -= deltaTime;
		}
		// Handle resizing directly.
		const glm::ivec2 screenSize = Input::manager().size();
		if(Input::manager().resized()) {
			atmosphereFramebuffer->resize(screenSize);
		}

		// Render.
		const glm::mat4 camToWorld = glm::inverse(camera.view());
		const glm::mat4 clipToCam  = glm::inverse(camera.projection());

		// Draw the atmosphere.
		glDisable(GL_DEPTH_TEST);
		atmosphereFramebuffer->bind();
		atmosphereFramebuffer->setViewport();
		GLUtilities::clearColor({0.0f, 0.0f, 0.0f, 1.0f});

		atmosphereProgram->use();
		const glm::mat4 camToWorldNoT = glm::mat4(glm::mat3(camToWorld));
		const glm::mat4 clipToWorld   = camToWorldNoT * clipToCam;
		atmosphereProgram->uniform("clipToWorld", clipToWorld);
		atmosphereProgram->uniform("viewPos", camera.position());
		atmosphereProgram->uniform("lightDirection", lightDirection);
		ScreenQuad::draw(precomputedScattering);
		atmosphereFramebuffer->unbind();

		// Tonemapping and final screen.
		GLUtilities::setViewport(0, 0, int(screenSize[0]), int(screenSize[1]));
		glEnable(GL_FRAMEBUFFER_SRGB);
		tonemapProgram->use();
		ScreenQuad::draw(atmosphereFramebuffer->textureId());
		glDisable(GL_FRAMEBUFFER_SRGB);

		// Settings.
		if(ImGui::DragFloat3("Light dir", &lightDirection[0], 0.05f, -1.0f, 1.0f)) {
			lightDirection = glm::normalize(lightDirection);
		}
		// Then render the interface.
		System::Gui::endFrame();
		//Display the result for the current rendering loop.
		glfwSwapBuffers(window);
	}

	// Cleaning.
	atmosphereFramebuffer->clean();

	// Clean the interface.
	System::Gui::clean();

	Resources::manager().clean();
	// Close GL context and any other GLFW resources.
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
