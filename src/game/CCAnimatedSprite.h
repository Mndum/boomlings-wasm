// CCAnimatedSprite: a node that plays a part-animation clip of a game type ("Block_g", "Bomb_r", ...).
// Frames are either one sprite frame or a stack of parts (CCPartAnimSprite) positioned per frame.
#pragma once
#include "cocos2d.h"
#include "AnimLibrary.h"
#include <functional>

class CCAnimatedSprite : public cocos2d::CCNode {
public:
    static CCAnimatedSprite* createWithType(const char* type);
    virtual bool initWithType(const char* type);
    // CCAnimatedSprite::loadType: switch the sprite to another type, keeping the current animation name
    void loadType(const char* type);
    // plays an animation; returns false (and calls onEnd right away) when the type has no such animation
    bool runAnimation(const std::string& name, bool loop = false, std::function<void()> onEnd = nullptr);
    const std::string& activeAnimation() const { return m_name; }
    const std::string& type() const { return m_type; }
    int frameIndex() const { return m_idx; }
    virtual void update(float dt);
    // opacity applied to every part sprite (the node itself has no opacity)
    void setPartOpacity(GLubyte o);
    // GamePiece overrides this like CCAnimatedSprite::animationFinished
    virtual void animationFinished(const std::string& name) { (void)name; }
protected:
    std::string m_type, m_name;
    const AnimClip* m_clip = NULL;
    int m_idx = 0;
    float m_acc = 0;
    bool m_loop = false;
    std::function<void()> m_onEnd;
    std::vector<cocos2d::CCSprite*> m_parts;
    cocos2d::CCNode* m_holder = NULL;
    GLubyte m_partOpacity = 255;
    void showFrame();
};
