#pragma once

#include "StenciledRenderer.hpp"

#include "Application.hpp"
#include "scene/Scene.hpp"
#include "renderers/Renderer.hpp"
#include "input/ControllableCamera.hpp"

#include "Common.hpp"

/**
 \brief Application for the stencil demo.
 \ingroup StencilDemo
 */
class StenciledApp final : public CameraApp {

public:
	/** Constructor.
	 \param config the configuration to apply when setting up
	 */
	explicit StenciledApp(RenderingConfig & config);

	/** \copydoc CameraApp::draw */
	void draw() override;

	/** \copydoc CameraApp::update */
	void update() override;

	/** \copydoc CameraApp::physics */
	void physics(double fullTime, double frameTime) override;

	/** \copydoc CameraApp::resize */
	void resize() override;

private:

	/** Select the scene to display.
	 \param scene the scene to use
	 */
	void setScene(const std::shared_ptr<Scene> & scene);

	std::unique_ptr<StenciledRenderer> _renderer; ///< Stenciled renderer.
	std::unique_ptr<Framebuffer> _finalRender; ///< The final render.
	
	std::vector<std::shared_ptr<Scene>> _scenes; ///< The existing scenes.
	std::vector<std::string> _sceneNames; ///< The associated scene names.
	size_t _currentScene = 0; ///< Currently selected scene.
};
