#include "processing/Blur.hpp"

Blur::Blur(){
	_passthroughProgram = Resources::manager().getProgram("passthrough");
}

void Blur::process(const GLuint ) {

}

void Blur::draw() {
	_passthroughProgram->use();
	ScreenQuad::draw(_finalTexture);
}

GLuint Blur::textureId() const {
	return _finalTexture;
}

void Blur::clean() const {
}


void Blur::resize(unsigned int , unsigned int ){
}

