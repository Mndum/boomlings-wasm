#include "CCAnimatedSprite.h"
USING_NS_CC;

CCAnimatedSprite* CCAnimatedSprite::createWithType(const char* type) {
    CCAnimatedSprite* s = new CCAnimatedSprite();
    if (s && s->initWithType(type)) { s->autorelease(); return s; }
    delete s;
    return NULL;
}
bool CCAnimatedSprite::initWithType(const char* type) {
    if (!CCNode::init()) return false;
    m_holder = CCNode::create();
    addChild(m_holder);
    m_type = type;
    scheduleUpdate();
    return true;
}
void CCAnimatedSprite::loadType(const char* type) {
    std::string keep = m_name.empty() ? "Idle01" : m_name;
    bool loop = m_loop;
    m_type = type;
    runAnimation(keep, loop, m_onEnd);
}
bool CCAnimatedSprite::runAnimation(const std::string& name, bool loop, std::function<void()> onEnd) {
    const AnimClip* c = AnimLibrary::shared()->get(m_type, name);
    if (!c) { if (onEnd) onEnd(); return false; }
    m_clip = c; m_name = name; m_idx = 0; m_acc = 0; m_loop = loop; m_onEnd = onEnd;
    showFrame();
    return true;
}
void CCAnimatedSprite::showFrame() {
    if (!m_clip || m_clip->frames.empty()) return;
    const AnimFrame& f = m_clip->frames[std::min<int>(m_idx, (int)m_clip->frames.size() - 1)];
    size_t n = f.single.empty() ? f.parts.size() : 1;
    while (m_parts.size() < n) {
        CCSprite* sp = new CCSprite();
        sp->init();
        sp->autorelease();
        m_holder->addChild(sp);
        m_parts.push_back(sp);
    }
    for (size_t i = 0; i < m_parts.size(); i++) { m_parts[i]->setVisible(i < n); m_parts[i]->setOpacity(m_partOpacity); }
    if (!f.single.empty()) {
        CCSpriteFrame* fr = CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(f.single.c_str());
        if (fr) {
            m_parts[0]->setDisplayFrame(fr);
            m_parts[0]->setPosition(CCPointZero); m_parts[0]->setScale(1); m_parts[0]->setRotation(0);
            m_parts[0]->setFlipX(false); m_parts[0]->setFlipY(false); m_parts[0]->setZOrder(0);
        }
        return;
    }
    for (size_t i = 0; i < f.parts.size(); i++) {
        const AnimPart& p = f.parts[i];
        CCSpriteFrame* fr = CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(p.tex.c_str());
        CCSprite* sp = m_parts[i];
        if (!fr) { sp->setVisible(false); continue; }
        sp->setDisplayFrame(fr);
        sp->setPosition(ccp(p.x, p.y)); sp->setScaleX(p.sx); sp->setScaleY(p.sy);
        sp->setRotation(p.rot); sp->setFlipX(p.fx); sp->setFlipY(p.fy); sp->setZOrder((int)i);
    }
}
#include <emscripten.h>
void CCAnimatedSprite::update(float dt) {
    static int freeze = emscripten_run_script_int("location.search.indexOf('freeze')>=0");
    if (!m_clip || freeze) return;
    m_acc += dt;
    while (m_acc >= m_clip->delay) {
        m_acc -= m_clip->delay;
        m_idx++;
        if (m_idx >= (int)m_clip->frames.size()) {
            if (m_loop) m_idx = 0;
            else {
                m_idx = (int)m_clip->frames.size() - 1;
                showFrame();
                std::function<void()> f = m_onEnd;
                m_onEnd = nullptr;
                std::string n = m_name;
                animationFinished(n);
                if (f) f();
                return;
            }
        }
        showFrame();
    }
}

void CCAnimatedSprite::setPartOpacity(GLubyte o) { m_partOpacity = o; for (CCSprite* s : m_parts) s->setOpacity(o); }
