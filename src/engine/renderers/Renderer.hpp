#pragma once
#include "system/Config.hpp"

/**
 \brief Base structure of a renderer.
 \ingroup Engine
 */
class Renderer {
	
public:
	
	/** Constructor.
	 \param config the configuration to apply when setting up
	 */
	explicit Renderer(RenderingConfig & config);
	
	/** Draw the scene and effects */
	virtual void draw() = 0;
	
	/** Perform once-per-frame update (buttons, GUI,...) */
	virtual void update();
	
	/** Perform physics simulation update.
	 \param fullTime the time elapsed since the beginning of the render loop
	 \param frameTime the duration of the last frame
	 \note This function can be called multiple times per frame.
	 */
	virtual void physics(double fullTime, double frameTime) = 0;
	
	/** Clean internal resources. */
	virtual void clean();
	
	/** Handle a window resize event.
	 \param width the new width
	 \param height the new height
	 */
	virtual void resize(unsigned int width, unsigned int height) = 0;
	
	/** Destructor */
	virtual ~Renderer() = default;

	/** Copy constructor.*/
	Renderer(const Renderer &) = default;

	/** Copy assignment. */
	Renderer& operator= (const Renderer&) = delete;

	/** Move constructor.*/
	Renderer(Renderer &&) = default;

	/** Move assignment. */
	Renderer& operator= (Renderer&&) = delete;

protected:
	
	/** Update the internal rendering resolution.
	 \param width the new width
	 \param height the new height
	 */
	void updateResolution(unsigned int width, unsigned int height);
	
	RenderingConfig & _config; ///< The current configuration.
	glm::vec2 _renderResolution = glm::vec2(0.0f); ///< The internal rendering resolution.
	
	
};

