#include "../PlayerCap.hpp"
#include <sys/mman.h>
#include <cstring>
#include <iostream>
int main() {
    auto* memory=static_cast<uint8_t*>(mmap(nullptr,4096,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_ANONYMOUS,-1,0));
    if(memory==MAP_FAILED)return 1;
    for(int solo=1;solo<=12;++solo)for(int duo=1;duo<=12;++duo) {
        auto manager=player_cap::manager_code({solo,duo});
        auto session=player_cap::session_code({solo,duo});
        for(int type=0;type<2;++type) {
            const auto& code=type?session:manager;
            const uint8_t prefix[]={0x89, uint8_t(type?0xfa:0xf8)};
            const uint8_t suffix[]={0x89,uint8_t(type?0xd0:0xc8),0xc3};
            std::memcpy(memory,prefix,2);std::memcpy(memory+2,code.data(),code.size());
            std::memcpy(memory+2+code.size(),suffix,3);
            for(int mode=0;mode<256;++mode) {
                const auto expected=mode==2?solo:mode==3?duo:12;
                if(reinterpret_cast<int(*)(int)>(memory)(mode)!=expected) {munmap(memory,4096);return 2;}
            }
        }
    }
    munmap(memory,4096);
    std::cout<<"PASS 73728 native Linux mode/limit combinations\n";
}
