// PauseLayer (decompiled PauseLayer.c, layout measured from the real game): a SlideInLayer - pauseBG panel rising from the
// bottom with a dark box (Time label, three buttons, music/effects sliders) and the play button; plus the FLAlertLayer-style
// confirm dialog. Coordinates are the JS port's 640x960 px converted to 320x480 y-up points.
#pragma once
#include "cocos2d.h"
#include <functional>
#include <string>
#include <vector>

// nine-slice of an hd texture: `capPx` hd pixels per corner, stretched to wPt x hPt points (anchor bottom-left)
cocos2d::CCNode* makeNineSlice(const char* file, float wPt, float hPt, float capPx);

class PauseLayer : public cocos2d::CCLayer {
public:
    static PauseLayer* create(int playSeconds, float music, float sfx);
    virtual bool init();
    std::function<void()> onResume, onMainMenu;
    std::function<void(float)> onMusic, onSfx;
    virtual void ccTouchesBegan(cocos2d::CCSet*, cocos2d::CCEvent*);
    virtual void ccTouchesMoved(cocos2d::CCSet*, cocos2d::CCEvent*);
    virtual void ccTouchesEnded(cocos2d::CCSet*, cocos2d::CCEvent*);
    virtual void update(float dt);
    int secs = 0;
    float vol[2] = { 1, 1 };
private:
    struct Btn { cocos2d::CCRect r; std::function<void()> fn; cocos2d::CCNode* node; };
    cocos2d::CCNode* slide = NULL;
    cocos2d::CCLayerColor* dim = NULL;
    cocos2d::CCNode* alertNode = NULL;
    cocos2d::CCLayerColor* alertDim = NULL;
    float alertT = 0;
    std::vector<Btn> btns, alertBtns;
    cocos2d::CCSprite* thumb[2] = { NULL, NULL };
    cocos2d::CCNode* fill[2] = { NULL, NULL };
    cocos2d::CCLabelBMFont* sliderLbl[2] = { NULL, NULL };
    int drag = -1;
    bool closing = false, opened = false;
    float tIn = 0;
    void layoutSlider(int i);
    void setSlider(int i, float xPt);
    void closePanel();
    void showAlert(const char* title, const char* msg, const std::vector<std::pair<std::string, std::function<void()>>>& buttons);
    void closeAlert();
    void addButton(const char* text, float cxPx, float cyPx, std::function<void()> fn);
};
