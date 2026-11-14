// Resolves objectDefinitions.plist + *_AnimDesc.plist into frame lists (CCAnimateFrameCache / SpriteAnimationManager::loadAnimations).
// Semantics follow the decompiled loader and are cross-checked against js/anim.js of the JS port.
#pragma once
#include "cocos2d.h"
#include <map>
#include <string>
#include <vector>

struct AnimPart { std::string tex; float x, y, sx, sy, rot; bool fx, fy; int z; };
struct AnimFrame { std::string single; std::vector<AnimPart> parts; };
struct AnimClip { float delay; bool looped; std::vector<AnimFrame> frames; };

class AnimLibrary {
public:
    static AnimLibrary* shared();
    void load(const char* objectDefinitions);
    // clip for type "Block_g" + animation "Idle01"; NULL when the animation does not exist
    const AnimClip* get(const std::string& type, const std::string& anim);
    bool hasType(const std::string& type) const { return m_defs->objectForKey(type) != NULL; }
private:
    cocos2d::CCDictionary* m_defs = NULL;
    std::map<std::string, cocos2d::CCDictionary*> m_descs;
    std::map<std::string, AnimClip*> m_cache;
    cocos2d::CCDictionary* desc(const std::string& file);
};
