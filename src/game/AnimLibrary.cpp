#include "AnimLibrary.h"
#include <sstream>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
USING_NS_CC;
using std::string;

static AnimLibrary* s_lib = NULL;
AnimLibrary* AnimLibrary::shared() { if (!s_lib) s_lib = new AnimLibrary(); return s_lib; }

static CCDictionary* D(CCObject* o) { return o ? dynamic_cast<CCDictionary*>(o) : NULL; }
static string S(CCDictionary* d, const char* k) {
    if (!d) return "";
    CCString* s = dynamic_cast<CCString*>(d->objectForKey(k));
    return s ? string(s->getCString()) : string();
}
static CCDictionary* K(CCDictionary* d, const char* k) { return d ? D(d->objectForKey(k)) : NULL; }
static string pad3(int i) { char b[16]; snprintf(b, sizeof b, "%03d", i); return b; }
static void nums(const string& s, float* out, int n) {
    // "{1.5, -2}" -> numbers
    int k = 0; const char* p = s.c_str();
    while (*p && k < n) { while (*p && *p != '-' && *p != '.' && (*p < '0' || *p > '9')) p++; if (!*p) break; char* e; out[k++] = (float)strtod(p, &e); p = e; }
}
static string replaceAll(string s, const string& a, const string& b) {
    for (size_t i = 0; (i = s.find(a, i)) != string::npos; i += b.size()) s.replace(i, a.size(), b);
    return s;
}

void AnimLibrary::load(const char* objectDefinitions) {
    m_defs = CCDictionary::createWithContentsOfFile(objectDefinitions);
    m_defs->retain();
}
CCDictionary* AnimLibrary::desc(const string& file) {
    auto it = m_descs.find(file); if (it != m_descs.end()) return it->second;
    CCDictionary* d = CCDictionary::createWithContentsOfFile(file.c_str()); if (d) d->retain();
    return m_descs[file] = d;
}

const AnimClip* AnimLibrary::get(const string& type, const string& animName) {
    string key = type + "|" + animName;
    auto c = m_cache.find(key); if (c != m_cache.end()) return c->second;
    m_cache[key] = NULL;
    CCDictionary* def = D(m_defs ? m_defs->objectForKey(type) : NULL);
    if (!def) return NULL;
    // "copy" types (Block_g -> Block_xyz): the copy source supplies animations and anim description
    string baseType = type, tag = S(def, "replaceTag");
    CCDictionary* src = NULL;
    if (def->objectForKey("copy")) { baseType = S(def, "copy"); src = D(m_defs->objectForKey(baseType)); }
    if (tag.empty() && type.size() > 2 && type[type.size() - 2] == '_') tag = type.substr(type.size() - 1);
    CCDictionary* anims = K(def, "animations"); CCDictionary* srcAnims = K(src, "animations");
    CCDictionary* anim = K(anims, animName.c_str()); if (!anim) anim = K(srcAnims, animName.c_str());
    if (!anim) return NULL;
    string animDesc = S(def, "animDesc"); if (animDesc.empty()) animDesc = S(src, "animDesc");
    // animation "copy": use a local sibling animation, or a GlobalAnimations entry
    string frameBase; string delayS = S(anim, "delay"), loopedS = S(anim, "looped"), framesS = S(anim, "frames");
    string cp = S(anim, "copy");
    if (!cp.empty()) {
        CCDictionary* local = K(anims, cp.c_str()); if (!local) local = K(srcAnims, cp.c_str());
        CCDictionary* srcA = (local && !local->objectForKey("copy")) ? local : K(D(m_defs->objectForKey("GlobalAnimations")), cp.c_str());
        if (srcA) {
            string fb = S(srcA, "animFrameName");
            if (!fb.empty()) frameBase = fb; else if (!local) frameBase = baseType + "_" + animName;
            if (delayS.empty()) delayS = S(srcA, "delay"); if (loopedS.empty()) loopedS = S(srcA, "looped"); if (framesS.empty()) framesS = S(srcA, "frames");
        }
    }
    AnimClip* clip = new AnimClip();
    clip->delay = delayS.empty() ? 0.0833f : (float)atof(delayS.c_str());
    clip->looped = loopedS == "1";
    std::vector<int> idx;
    if (!framesS.empty()) {
        if (framesS.find(' ') != string::npos) { std::istringstream is(framesS); int v; while (is >> v) idx.push_back(v); }
        else { int n = atoi(framesS.c_str()); for (int i = 1; i <= n; i++) idx.push_back(i); }
    } else idx.push_back(1);
    CCDictionary* dsc = animDesc.empty() ? NULL : desc(animDesc);
    CCDictionary* cont = K(dsc, "animationContainer");
    for (int i : idx) {
        AnimFrame f;
        string partsKey = baseType + "_" + animName + "_" + pad3(i) + ".png";
        CCDictionary* pd = frameBase.empty() ? K(cont, partsKey.c_str()) : NULL;
        if (pd) {
            CCDictElement* el = NULL;
            CCDICT_FOREACH(pd, el) {
                CCDictionary* p = D(el->getObject()); if (!p) continue;
                AnimPart ap; float v[2];
                nums(S(p, "position"), v, 2); ap.x = v[0]; ap.y = v[1];
                nums(S(p, "scale"), v, 2); ap.sx = v[0]; ap.sy = v[1];
                nums(S(p, "flipped"), v, 2); ap.fx = v[0] != 0; ap.fy = v[1] != 0;
                ap.rot = (float)atof(S(p, "rotation").c_str()); ap.z = atoi(S(p, "zValue").c_str());
                ap.tex = replaceAll(S(p, "texture"), "xyz", tag.empty() ? "xyz" : tag);
                f.parts.push_back(ap);
            }
            std::stable_sort(f.parts.begin(), f.parts.end(), [](const AnimPart& a, const AnimPart& b) { return a.z < b.z; });
        } else {
            string fb = frameBase.empty() ? type : replaceAll(frameBase, "xyz", tag.empty() ? "xyz" : tag);
            string cand = frameBase.empty() ? type + "_" + animName + "_" + pad3(i) + ".png" : fb + "_" + pad3(i) + ".png";
            if (CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(cand.c_str())) f.single = cand;
            else { cand = type + "_" + pad3(i) + ".png"; if (CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(cand.c_str())) f.single = cand; }
        }
        clip->frames.push_back(f);
    }
    m_cache[key] = clip;
    return clip;
}
