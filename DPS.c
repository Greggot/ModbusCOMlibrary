#include "DPS.h"
#include "Modbuslib.h"

uint8_t isNumber(char c)
{
    return c < 0x3A && c > 0x2F;
}

int main(int argc, char* argv[])    // COM baud 
{
    if(argc < 6)
    {
        printf("Wrong arguments (-0x1)\nUse DPS.exe COM baudrate block_n Voltage Amperage");
        return -1;
    }

    if(!isNumber(argv[1][7]))
    {
        printf("Wrong COM port format (-0x4) - %c\n", argv[1][7]);
        return -4;
    }

    uint32_t baudrate = strtol(argv[2], NULL, 10);
    uint8_t block_addr = strtol(argv[3], NULL, 10);

    HANDLE port = open_serial_port(argv[1], baudrate);
    if(port == INVALID_HANDLE_VALUE)
    {
        printf("Unable open COM port (-0x5)\n");
        return -5;
    }

    char* message = (char*)malloc(sizeof(char) * 0x20);

    uint16_t volt = strtol(argv[4], NULL, 10);
    Modbus_Write_SingleRegister(port, 1, USET, volt);
    uint16_t amp = strtol(argv[5], NULL, 10);
    Modbus_Write_SingleRegister(port, 1, ISET, amp);

    printf("Model = %i, Version = %i\n", 
    Modbus_DPS_Model(port, block_addr, message, 20), 
    Modbus_DPS_Version(port, block_addr, message, 20));

    printf("UIN = %i\n", Modbus_getValueUI16(port, 1, UIN, message, 20));
    uint32_t VA = 0;

    while(1)
    {
        VA = Modbus_getValueUI32(port, 1, 2, message, 20);
        printf("V = %i, A = %i\n", VA >> 16, VA & 0xFFFF);
    }

    return 0;
}