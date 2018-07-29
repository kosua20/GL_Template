#include "DirectionalLight.hpp"

#include <stdio.h>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>





DirectionalLight::DirectionalLight(const glm::vec3& worldDirection, const glm::vec3& color, const float extent, const float near, const float far) : Light(color) {
	
	_lightDirection = worldDirection;
	_projectionMatrix = glm::ortho(-extent,extent,-extent,extent,near,far);
	_viewMatrix = glm::lookAt(-_lightDirection, glm::vec3(0.0f,0.0f,0.0f), glm::vec3(0.0f,1.0f,0.0f));
	_mvp = _projectionMatrix * _viewMatrix;
}


void DirectionalLight::init(const std::map<std::string, GLuint>& textureIds){
	// Setup the framebuffer.
	_shadowPass = std::make_shared<Framebuffer>(512, 512, GL_RG,GL_FLOAT, GL_RG16F, GL_LINEAR,GL_CLAMP_TO_BORDER, true);
	_blurPass = std::make_shared<Framebuffer>(_shadowPass->width(), _shadowPass->height(), GL_RG,GL_FLOAT, GL_RG16F, GL_LINEAR,GL_CLAMP_TO_BORDER, false);
	_blurScreen.init(_shadowPass->textureId(), "box-blur-2");
	
	std::map<std::string, GLuint> textures = textureIds;
	textures["shadowMap"] = _blurPass->textureId();
	_screenquad.init(textures, "directional_light");
	
}

void DirectionalLight::draw(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::vec2& invScreenSize ) const {
	
	
	glm::mat4 viewToLight = _mvp * glm::inverse(viewMatrix);
	// Store the four variable coefficients of the projection matrix.
	glm::vec4 projectionVector = glm::vec4(projectionMatrix[0][0], projectionMatrix[1][1], projectionMatrix[2][2], projectionMatrix[3][2]);
	glm::vec3 lightDirectionViewSpace = glm::vec3(viewMatrix * glm::vec4(_lightDirection, 0.0));
	
	glUseProgram(_screenquad.program()->id());
	
	glUniform3fv(_screenquad.program()->uniform("lightDirection"), 1,  &lightDirectionViewSpace[0]);
	glUniform3fv(_screenquad.program()->uniform("lightColor"), 1,  &_color[0]);
	// Projection parameter for position reconstruction.
	glUniform4fv(_screenquad.program()->uniform("projectionMatrix"), 1, &(projectionVector[0]));
	glUniformMatrix4fv(_screenquad.program()->uniform("viewToLight"), 1, GL_FALSE, &viewToLight[0][0]);

	_screenquad.draw();

}

void DirectionalLight::bind() const {
	_shadowPass->bind();
	_shadowPass->setViewport();
	
	// Set the clear color to white.
	glClearColor(1.0f,1.0f,1.0f,0.0f);
	// Clear the color and depth buffers.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void DirectionalLight::blurAndUnbind() const {
	// Unbind the shadow map framebuffer.
	_shadowPass->unbind();
	// ----------------------
	
	// --- Blur pass --------
	glDisable(GL_DEPTH_TEST);
	// Bind the post-processing framebuffer.
	_blurPass->bind();
	// Set screen viewport.
	_blurPass->setViewport();
	// Draw the fullscreen quad
	_blurScreen.draw();
	 
	_blurPass->unbind();
	glEnable(GL_DEPTH_TEST);
}

void DirectionalLight::update(const glm::vec3 & newDirection){
	_lightDirection = newDirection;
	_viewMatrix = glm::lookAt(-_lightDirection, glm::vec3(0.0f,0.0f,0.0f), glm::vec3(0.0f,1.0f,0.0f));
	_mvp = _projectionMatrix * _viewMatrix;
}

void DirectionalLight::clean() const {
	_blurPass->clean();
	_blurScreen.clean();
	_shadowPass->clean();
}

