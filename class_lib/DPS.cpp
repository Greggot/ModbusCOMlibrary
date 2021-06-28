#include <iostream>
#include "Modbuslib.hpp"

int main(int argc, char* argv[])
{
    if(argc < 6)
    {
        std::cout << "Wrong arguments(-0x1)\nUse DPS.exe  COM_n\n";
        return -1;
    }
    ModbusDeviceDPS first(2, strtol(argv[1], NULL, 10), 19200);
    ModbusDeviceDPS second(1, first.getPort());
    std::cout  << "Model - " << first.Model() << ", Version - " << first.Version() << '\n';
    std::cout  << "Uin = " << first.Uin() << '\n';
    std::cout  << "Current VA: V = " << first.Uset() << ", I = " << first.Iset() << '\n';

    first.Uset(strtol(argv[2], NULL, 10));
    first.Iset(strtol(argv[3], NULL, 10));

    second.Uset(strtol(argv[4], NULL, 10));
    second.Iset(strtol(argv[5], NULL, 10));

    if(!first.IsON())
        first.ON();
    else
        first.OFF();
    
    uint8_t count = 0;
    uint32_t VA = 0;
    while(1)
    {  
        VA = first.getVA(); 
        if(count == 0x10)
            break;
        if(!(VA & 0xFF))
        {
            count++;
            std::cout << "There\'s no load\n";
        }
        else
            //std::cout  << "Current VA: V = " << first.Uout() << ", I = " << first.Iout() << '\n';
            std::cout << "Current VA: V = " << (VA >> 16) << ", I = " << (VA & 0xFF) << '\n';
    }
    return 0;
}