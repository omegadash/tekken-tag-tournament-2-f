#include "tekken_force_camera_diagnostics.h"
#include "kernel_shared.h"
#include <array>
#include <bit>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <set>

namespace {
std::mutex traceMutex;
float GReadF32(uint8_t* base,uint32_t address)
{
    return std::bit_cast<float>(GRead32(base,address));
}
FILE* TraceFile()
{
    static FILE* file=[] {
        const char* path=std::getenv("TTT2_TF_CAMERA_TRACE");
        return path && *path ? std::fopen(path,"w") : nullptr;
    }();
    return file;
}
double Milliseconds()
{
    return std::chrono::duration<double,std::milli>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}
void Words(FILE* file,uint8_t* base,uint32_t address)
{
    for(unsigned i=0;i<16;++i) std::fprintf(file,"%s%u",i?",":"",GRead32(base,address+i*4));
}
}

bool TekkenForceStationaryCamera()
{
    static const bool stationary=[] {
        const char* mode=std::getenv("TTT2_TF_CAMERA");
        return mode && std::strcmp(mode,"stationary")==0;
    }();
    return stationary;
}
bool TekkenForceCameraTraceEnabled()
{
    static const bool enabled=[] {const char* p=std::getenv("TTT2_TF_CAMERA_TRACE");return p && *p;}();
    return enabled;
}
bool TekkenForceLegacyCamera()
{
    static const bool legacy=[] {const char* p=std::getenv("TTT2_TF_CAMERA");return p && std::strcmp(p,"legacy")==0;}();
    return legacy;
}
void TekkenForceTraceCamera(const char* phase,uint8_t* base,uint32_t camera,
    uint32_t player,uint64_t tick,const float* eye,const float* target)
{
    if(!TekkenForceCameraTraceEnabled()) return;
    std::lock_guard lock(traceMutex);
    FILE* file=TraceFile();if(!file) return;
    std::fprintf(file,"{\"type\":\"%s\",\"ms\":%.3f,\"tick\":%llu,\"stationary\":%u,\"root\":[%.6f,%.6f,%.6f],\"pose\":[%.6f,%.6f,%.6f],\"eye\":[%.6f,%.6f,%.6f],\"target\":[%.6f,%.6f,%.6f],\"view\":[",
        phase,Milliseconds(),static_cast<unsigned long long>(tick),unsigned(TekkenForceStationaryCamera()),
        GReadF32(base,player),GReadF32(base,player+4),GReadF32(base,player+8),
        GReadF32(base,player+20),GReadF32(base,player+24),GReadF32(base,player+28),
        eye[0],eye[1],eye[2],target[0],target[1],target[2]);
    Words(file,base,camera+224);std::fputs("],\"projection\":[",file);
    Words(file,base,camera+160);std::fputs("],\"vp\":[",file);
    Words(file,base,camera+96);std::fputs("]}\n",file);
}
void TekkenForceTraceDraw(const uint32_t* regs,uint32_t shader,uint64_t frame)
{
    if(!TekkenForceCameraTraceEnabled()) return;
    // CP thread only. Record each distinct c8-c11 matrix once per frame/shader.
    static uint64_t lastFrame=~uint64_t(0);
    static std::set<std::array<uint32_t,17>> seen;
    if(frame!=lastFrame){seen.clear();lastFrame=frame;}
    std::array<uint32_t,17> key{};key[0]=shader;
    for(unsigned i=0;i<16;++i) key[i+1]=regs[0x4000+8*4+i];
    if(!seen.insert(key).second) return;
    std::lock_guard lock(traceMutex);FILE* file=TraceFile();if(!file) return;
    std::fprintf(file,"{\"type\":\"draw\",\"ms\":%.3f,\"frame\":%llu,\"shader\":%u,\"pitch\":%u,\"depth\":%u,\"vp\":[",
        Milliseconds(),static_cast<unsigned long long>(frame),shader,regs[0x2000]&0x3FFF,regs[0x2200]);
    for(unsigned i=0;i<16;++i) std::fprintf(file,"%s%u",i?",":"",key[i+1]);
    std::fputs("]}\n",file);
}
