#include "scene/Object.hpp"


Object::Object() {}

Object::Object(const Object::Type type, const Mesh * mesh, bool castShadows){
	_material = type;
	_castShadow = castShadows;
	_mesh = mesh;
}

void Object::decode(const KeyValues & params, const Storage mode){
	
#define REGISTER_TYPE(type) {#type, Type::type}
	const std::map<std::string, Object::Type> types = {
		REGISTER_TYPE(Common),
		REGISTER_TYPE(PBRRegular),
		REGISTER_TYPE(PBRParallax),
		REGISTER_TYPE(PBRNoUVs)
	};
#undef REGISTER_TYPE
	
	// We expect there is only one transformation in the parameters set.
	_model = Codable::decodeTransformation(params.elements);
	
	for(const auto & param : params.elements){
		if(param.key == "type" && !param.values.empty()){
			const std::string typeString = param.values[0];
			if(types.count(typeString) > 0){
				_material = types.at(typeString);
			}
			
		} else if(param.key == "mesh" && !param.values.empty()){
			const std::string meshString = param.values[0];
			_mesh = Resources::manager().getMesh(meshString, mode);
			
		} else if(param.key == "shadows"){
			_castShadow = Codable::decodeBool(param);
			
		} else if(param.key == "textures"){
			for(const auto & paramTex : param.elements){
				const TextureInfos * tex = Codable::decodeTexture(paramTex, mode);
				addTexture(tex);
			}
			
		} else if(param.key == "animations"){
			_animations = Animation::decode(param.elements);
		} else if(param.key == "twosided"){
			_twoSided = Codable::decodeBool(param);
		} else if(param.key == "masked"){
			_masked = Codable::decodeBool(param);
		}
	}
}

void Object::addTexture(const TextureInfos * infos){
	_textures.push_back(infos);
}

void Object::addAnimation(std::shared_ptr<Animation> anim){
	_animations.push_back(anim);
}

void Object::update(double fullTime, double frameTime) {
	glm::mat4 model = _model;
	for(auto anim : _animations){
		model = anim->apply(model, fullTime, frameTime);
	}
	_model = model;
}

BoundingBox Object::boundingBox() const {
	return _mesh->bbox.transformed(_model);
}


