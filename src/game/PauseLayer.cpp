#include "PauseLayer.h"
#include <cmath>
USING_NS_CC;

static CCPoint P(float xPx, float yPx) { return ccp(xPx / 2, 480 - yPx / 2); }   // JS canvas px (y down) -> design points (y up)
static const float kSlide = 0.5f, kSlideDist = 1066.7f / 2;

static float easeInOut2(float u) { return u < 0.5f ? 0.5f * powf(2 * u, 2) : 1 - 0.5f * powf(2 - 2 * u, 2); }
static float elastic(float u, float p) { if (u <= 0) return 0; if (u >= 1) return 1; return powf(2, -10 * u) * sinf((u - p / 4) * 2 * 3.14159265f / p) + 1; }

// sprite showing a pixel rectangle of an hd texture (setTextureRect takes points on this build; CCSprite::create(file, rect) is unreliable)
static CCSprite* pieceSprite(const char* file, CCRect rectPx) {
    CCSprite* s = CCSprite::create(file);
    float k = 1.0f / CC_CONTENT_SCALE_FACTOR();   // CCSprite::setTextureRect takes points
    s->setTextureRect(CCRectMake(rectPx.origin.x * k, rectPx.origin.y * k, rectPx.size.width * k, rectPx.size.height * k));
    return s;
}

CCNode* makeNineSlice(const char* file, float wPt, float hPt, float capPx) {
    CCNode* n = CCNode::create();
    CCSprite* probe = CCSprite::create(file);
    if (!probe) return n;
    CCSize tp = probe->getTexture()->getContentSizeInPixels();
    float csf = CC_CONTENT_SCALE_FACTOR(), cp = capPx;
    float xs[4] = { 0, cp, tp.width - cp, tp.width }, ys[4] = { 0, cp, tp.height - cp, tp.height };
    float xo[4] = { 0, cp / csf, wPt - cp / csf, wPt };                // piece edges in points (x from the left)
    float yo[4] = { hPt, hPt - cp / csf, cp / csf, 0 };                 // y from the bottom, top row first
    for (int r = 0; r < 3; r++) for (int c = 0; c < 3; c++) {
        CCRect rect(xs[c], ys[r], xs[c + 1] - xs[c], ys[r + 1] - ys[r]);
        CCSprite* s = pieceSprite(file, rect);
        CCSize cs = s->getContentSize();
        s->setAnchorPoint(ccp(0, 0));
        s->setScaleX((xo[c + 1] - xo[c]) / cs.width); s->setScaleY((yo[r] - yo[r + 1]) / cs.height);
        s->setPosition(ccp(xo[c], yo[r + 1]));
        n->addChild(s);
    }
    n->setContentSize(CCSizeMake(wPt, hPt));
    return n;
}

// rounded dark box with a vertical gradient (#00304c, #003750 at 35%, #001f31), drawn as per-row triangle strips
class GradientBox : public CCNode {
public:
    float w = 260, h = 370, r = 8;
    bool init() { if (!CCNode::init()) return false; setShaderProgram(CCShaderCache::sharedShaderCache()->programForKey(kCCShader_PositionColor)); return true; }
    CREATE_FUNC(GradientBox);
    static void col(float t, float* c) {
        static const float a[3] = { 0x00 / 255.f, 0x30 / 255.f, 0x4c / 255.f }, b[3] = { 0x00 / 255.f, 0x37 / 255.f, 0x50 / 255.f }, d[3] = { 0x00 / 255.f, 0x1f / 255.f, 0x31 / 255.f };
        const float *p0 = t < 0.35f ? a : b, *p1 = t < 0.35f ? b : d; float u = t < 0.35f ? t / 0.35f : (t - 0.35f) / 0.65f;
        for (int i = 0; i < 3; i++) c[i] = p0[i] + (p1[i] - p0[i]) * u;
    }
    virtual void draw() {
        CC_NODE_DRAW_SETUP();
        ccGLBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        ccGLEnableVertexAttribs(kCCVertexAttribFlag_Position | kCCVertexAttribFlag_Color);
        const int R = 74;
        ccVertex2F v[(R + 1) * 2]; ccColor4F c[(R + 1) * 2];
        for (int i = 0; i <= R; i++) {
            float t = (float)i / R, dTop = t * h, dBot = (1 - t) * h, d = std::min(dTop, dBot), inset = 0;
            if (d < r) inset = r - sqrtf(r * r - (r - d) * (r - d));
            float y = h - dTop, cc[3]; col(t, cc);
            v[i * 2].x = inset; v[i * 2].y = y; v[i * 2 + 1].x = w - inset; v[i * 2 + 1].y = y;
            c[i * 2] = c[i * 2 + 1] = ccc4f(cc[0], cc[1], cc[2], 1);
        }
        glBindBuffer(GL_ARRAY_BUFFER, 0); glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glVertexAttribPointer(kCCVertexAttrib_Position, 2, GL_FLOAT, GL_FALSE, 0, v);
        glVertexAttribPointer(kCCVertexAttrib_Color, 4, GL_FLOAT, GL_FALSE, 0, c);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, (R + 1) * 2);
    }
};

PauseLayer* PauseLayer::create(int playSeconds, float music, float sfx) {
    PauseLayer* l = new PauseLayer();
    l->secs = playSeconds; l->vol[0] = music; l->vol[1] = sfx;
    if (l && l->init()) { l->autorelease(); return l; }
    delete l; return NULL;
}

static CCLabelBMFont* label(const char* text, const char* font, float scale, CCPoint pos, CCPoint anchor = ccp(0.5f, 0.5f)) {
    CCLabelBMFont* l = CCLabelBMFont::create(text, font);
    l->setScale(scale); l->setAnchorPoint(anchor); l->setPosition(pos);
    return l;
}

void PauseLayer::addButton(const char* text, float cxPx, float cyPx, std::function<void()> fn) {
    float w = 216, h = 33;
    CCNode* bg = makeNineSlice("buttonBG2-hd.png", w, h, 28);
    CCPoint c = P(cxPx, cyPx);
    bg->setPosition(ccp(c.x - w / 2, c.y - h / 2));
    slide->addChild(bg, 2);
    slide->addChild(label(text, "goldFont-hd.fnt", 1.0f, ccp(c.x, c.y - 4)), 3);
    btns.push_back({ CCRectMake(c.x - w / 2, c.y - h / 2, w, h), fn, bg });
}

void PauseLayer::layoutSlider(int i) {
    float gx = 113, gw = 420, gy = i == 0 ? 660 : 780, v = vol[i];
    float txPx = gx + (gw - 13) * v;
    char b[32]; sprintf(b, "%s, %d%%", i == 0 ? "Music" : "Effects", (int)floorf(v * 100 + 0.5f));
    sliderLbl[i]->setString(b);
    // filled part: tiles of sliderBar from gx+8 to the thumb, vertically cropped by 3.3px
    fill[i]->removeAllChildren();
    CCSprite* probe = CCSprite::create("sliderBar-hd.png");
    CCSize tp = probe->getTexture()->getContentSizeInPixels();
    float fillPx = std::max(0.f, txPx - gx - 8), done = 0;
    while (done < fillPx - 0.01f) {
        float wPx = std::min(tp.width, fillPx - done);
        CCSprite* t = pieceSprite("sliderBar-hd.png", CCRectMake(0, 3.3f, wPx, tp.height - 6.6f));
        t->setAnchorPoint(ccp(0, 0.5f)); t->setPosition(P(gx + 8 + done, gy));
        fill[i]->addChild(t);
        done += wPx;
    }
    CCTexture2D* tx = CCTextureCache::sharedTextureCache()->addImage(drag == i ? "sliderthumbsel-hd.png" : "sliderthumb-hd.png");
    CCSize ts = tx->getContentSize();   // points (setTextureRect takes points)
    thumb[i]->setTexture(tx); thumb[i]->setTextureRect(CCRectMake(0, 0, ts.width, ts.height));
    thumb[i]->setPosition(P(txPx, gy));
}

bool PauseLayer::init() {
    if (!CCLayer::init()) return false;
    dim = CCLayerColor::create(ccc4(0, 0, 0, 0)); addChild(dim, 0);
    slide = CCNode::create(); addChild(slide, 1);
    CCSprite* bg = CCSprite::createWithSpriteFrameName("pauseBG_001.png");
    bg->setPosition(P(320, 16 + 924 / 2 + 19.6f)); slide->addChild(bg, 0);
    GradientBox* box = GradientBox::create(); box->w = 260; box->h = 370; box->r = 8;
    box->setPosition(ccp(30, 480 - 79.85f - 370)); slide->addChild(box, 1);
    int hh = secs / 3600, mm = secs / 60 % 60, ss = secs % 60;
    char tm[32]; sprintf(tm, "%02d:%02d:%02d", hh, mm, ss);
    slide->addChild(label("Time:", "goldFont-hd.fnt", 0.79f, P(307, 221.33f), ccp(1, 0.5f)), 3);
    slide->addChild(label(tm, "numbersFont-hd.fnt", 0.7f, P(320, 219.33f), ccp(0, 0.5f)), 3);
    addButton("Main Menu", 320, 293.63f, [this] {
        showAlert("Restart?", "Restart the game from Level 1?", { { "CANCEL", [] {} }, { "OK", [this] { if (onMainMenu) onMainMenu(); } } });
    });
    addButton("Free Gold", 320, 400, [this] { showAlert("Error", "Something went wrong.\nPlease try again later!", { { "OK", [] {} } }); });
    addButton("Upgrade", 320, 506, [this] { showAlert("Error", "Something went wrong.\nPlease try again later!", { { "OK", [] {} } }); });
    for (int i = 0; i < 2; i++) {
        float gy = i == 0 ? 660 : 780;
        CCSprite* gr = CCSprite::create("slidergroove-hd.png");
        gr->setAnchorPoint(ccp(0, 0.5f)); gr->setPosition(P(113, gy)); slide->addChild(gr, 2);
        fill[i] = CCNode::create(); slide->addChild(fill[i], 3);
        sliderLbl[i] = label("", "goldFont-hd.fnt", 0.79f, P(320, gy - 39.85f)); slide->addChild(sliderLbl[i], 3);
        thumb[i] = CCSprite::create("sliderthumb-hd.png"); thumb[i]->setScale(0.98f); slide->addChild(thumb[i], 4);
    }
    CCSprite* play = CCSprite::createWithSpriteFrameName("playBtn_001.png");
    play->setPosition(P(320, 60)); play->setScale(0.852f); slide->addChild(play, 3);
    btns.push_back({ CCRectMake(160 - 34, 450 - 34, 68, 68), [this] { closePanel(); }, play });
    for (int i = 0; i < 2; i++) layoutSlider(i);
    slide->setPositionY(-kSlideDist);
    setTouchEnabled(true);
    scheduleUpdate();
    return true;
}

void PauseLayer::update(float dt) {
    tIn += dt;
    float u = std::min(1.f, tIn / kSlide);
    float k = easeInOut2(closing ? 1 - u : u), dk = closing ? 1 - u : u;
    slide->setPositionY(-(1 - k) * kSlideDist);
    dim->setOpacity((GLubyte)(140 * dk));
    if (!opened && !closing && u >= 1) opened = true;
    if (closing && tIn > kSlide) {
        unscheduleUpdate();
        std::function<void()> f = onResume;
        removeFromParent();
        if (f) f();
        return;
    }
    if (alertNode) {
        float a = std::min(1.f, alertT / 0.5f);
        alertNode->setScale(std::max(0.f, elastic(a, 0.6f)));
        alertDim->setOpacity((GLubyte)(150 * std::min(1.f, alertT / 0.14f)));
        alertT += dt;
    }
}

void PauseLayer::closePanel() { if (closing) return; closing = true; tIn = 0; }

void PauseLayer::closeAlert() {
    if (alertNode) { alertNode->removeFromParent(); alertNode = NULL; alertDim->removeFromParent(); alertDim = NULL; }
    alertBtns.clear();
}

void PauseLayer::showAlert(const char* title, const char* msg, const std::vector<std::pair<std::string, std::function<void()>>>& buttons) {
    closeAlert();
    alertDim = CCLayerColor::create(ccc4(0, 0, 0, 0)); addChild(alertDim, 10);
    alertNode = CCNode::create(); alertNode->setPosition(ccp(160, 240)); addChild(alertNode, 11);
    alertT = 0;
    float w = 495 / 2.f, h = 274.7f / 2;
    CCNode* box = makeNineSlice("square01_001-hd.png", w, h, 56);
    box->setPosition(ccp(-w / 2, -h / 2 + 0.9f)); alertNode->addChild(box);
    alertNode->addChild(label(title, "goldFont-hd.fnt", 0.9f, ccp(0, 37)), 2);
    CCLabelBMFont* m = label(msg, "chatFont-hd.fnt", 0.99f, ccp(0, 15), ccp(0.5f, 0.5f));
    m->setAlignment(kCCTextAlignmentCenter); alertNode->addChild(m, 2);
    std::vector<float> ws; float tot = 0;
    std::vector<CCLabelBMFont*> ls;
    for (auto& b : buttons) {
        CCLabelBMFont* l = CCLabelBMFont::create(b.first.c_str(), "goldFont-hd.fnt");
        float tw = l->getContentSize().width * 0.8f;
        ws.push_back(std::max(109.f, tw * 2 + 33) / 2); ls.push_back(l); tot += ws.back();
    }
    float gap = 33 / 2.f; tot += gap * (buttons.size() - 1);
    float bx = -tot / 2, by = -40.65f + 0.f;
    for (size_t i = 0; i < buttons.size(); i++) {
        float bw = ws[i], bh = 61 / 2.f;
        CCNode* bg = makeNineSlice("buttonBG2-hd.png", bw, bh, 28);
        bg->setPosition(ccp(bx, by - bh / 2)); alertNode->addChild(bg, 2);
        CCLabelBMFont* l = ls[i];
        float sc = std::min(1.f, (bw * 2 - 36) / (l->getContentSize().width * 2));
        l->setScale(sc); l->setPosition(ccp(bx + bw / 2, by - 3)); alertNode->addChild(l, 3);
        std::function<void()> fn = buttons[i].second;
        CCPoint c0 = alertNode->getPosition();
        alertBtns.push_back({ CCRectMake(c0.x + bx, c0.y + by - bh / 2, bw, bh), [this, fn] { closeAlert(); if (fn) fn(); }, bg });
        bx += bw + gap;
    }
}

void PauseLayer::setSlider(int i, float xPt) {
    float v = std::max(0.f, std::min(1.f, (xPt * 2 - 113) / (420 - 13)));
    vol[i] = v; layoutSlider(i);
    if (i == 0 && onMusic) onMusic(v);
    if (i == 1 && onSfx) onSfx(v);
}

void PauseLayer::ccTouchesBegan(CCSet* t, CCEvent*) {
    if (closing || !opened) return;
    CCTouch* tc = (CCTouch*)*t->begin();
    CCPoint p = convertToNodeSpace(tc->getLocation());
    if (alertNode) { for (auto& b : alertBtns) if (b.r.containsPoint(p)) { auto fn = b.fn; fn(); return; } return; }
    for (int i = 0; i < 2; i++) {
        float gy = i == 0 ? 660 : 780;
        CCRect r((113 - 20) / 2.f, 480 - (gy + 40) / 2, (420 + 40) / 2.f, 40);
        if (r.containsPoint(p)) { drag = i; setSlider(i, p.x); return; }
    }
    for (auto& b : btns) if (b.r.containsPoint(p)) { auto fn = b.fn; fn(); return; }
}

void PauseLayer::ccTouchesMoved(CCSet* t, CCEvent*) {
    if (drag < 0) return;
    CCTouch* tc = (CCTouch*)*t->begin();
    setSlider(drag, convertToNodeSpace(tc->getLocation()).x);
}

void PauseLayer::ccTouchesEnded(CCSet*, CCEvent*) {
    if (drag >= 0) { int d = drag; drag = -1; layoutSlider(d); }
}
