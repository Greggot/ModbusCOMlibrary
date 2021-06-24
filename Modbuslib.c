#include "Modbuslib.h"

uint16_t MODBUS_CRC(char* str, uint8_t size)
{
    uint8_t nTemp;
    uint16_t CRC = 0xFFFF;

    while (size--)
    {
        nTemp = *str++ ^ CRC;
        CRC >>= 8;
        CRC  ^= MODBUS_CRC_Table[(nTemp & 0xFF)];
    }
    return (CRC & 0xFF00) >> 8 | (CRC & 0xFF) << 8;
}



int write_port(HANDLE port, uint8_t * buffer, size_t size)
{
    Sleep(4);
    DWORD written;
    if (!WriteFile(port, buffer, size, &written, NULL))
    {
        return -1;
    }
    if (written != size)
        return -1;
    return 0;
}

__declspec(dllexport) HANDLE open_serial_port(const char * device, uint32_t baud_rate)
{
    //Sleep(4);

    HANDLE port = CreateFileA(device, GENERIC_READ | GENERIC_WRITE, 0, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (port == INVALID_HANDLE_VALUE)
        return INVALID_HANDLE_VALUE;
    
    // Flush away any bytes previously read or written.
    BOOL success = FlushFileBuffers(port);
    if (!success)
    {
        printf("Failed to flush serial port\n");
        CloseHandle(port);
        return INVALID_HANDLE_VALUE;
    }
    
    // Configure read and write operations to time out after 100 ms.
    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = 0;
    timeouts.ReadTotalTimeoutConstant = 20;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 20;
    timeouts.WriteTotalTimeoutMultiplier = 0;
    
    success = SetCommTimeouts(port, &timeouts);
    if (!success)
    {
        printf("Failed to set serial timeouts\n");
        CloseHandle(port);
        return INVALID_HANDLE_VALUE;
    }
    
    // Set the baud rate and other options.
    DCB state = {0};
    state.DCBlength = sizeof(DCB);
    state.BaudRate = baud_rate;
    state.ByteSize = 8;
    state.Parity = NOPARITY;
    state.StopBits = ONESTOPBIT;
    success = SetCommState(port, &state);
    if (!success)
    {
        printf("Failed to set serial settings\n");
        CloseHandle(port);
        return INVALID_HANDLE_VALUE;
    }

    return port;
}

SSIZE_T read_port(HANDLE port, uint8_t * buffer, size_t size)
{
    DWORD received;
    if (!ReadFile(port, buffer, size, &received, NULL))
        return -1;

    return received;
}

__declspec(dllexport) void Modbus_DPS_ON(HANDLE port, uint8_t block_addr)
{
    char message[8] = {block_addr, MdBus_WRITE, 0x0, ONOFF, 0x0, 0x01, 0x98, 0x08};
    write_port(port, message, 8);
}

__declspec(dllexport) void Modbus_DPS_OFF(HANDLE port, uint8_t block_addr)
{
    char message[8] = {block_addr, MdBus_WRITE, 0x0, ONOFF, 0x0, 0x0, 0x59, 0xC8};
    write_port(port, message, 8);
}

__declspec(dllexport) uint16_t Modbus_getValueUI16(HANDLE port, uint8_t block_addr, uint16_t reg, uint8_t* buffer, uint16_t size)
{
    read_port(port, buffer, size);

    char message[8] = {block_addr, MdBus_READ, reg >> 8, reg & 0xFF, 0x0, 0x1, 0x0, 0x0};
    uint16_t CRC = MODBUS_CRC(message, 6);
    message[6] = CRC >> 8;
    message[7] = CRC & 0xFF;

    uint16_t in_size = 0;
    uint8_t count = 0;
    write_port(port, message, 8);
    
    while(!in_size)
    {
        count++;
        in_size = read_port(port, buffer, size);
        if(count == 0x10)
            return 0;
    }
    return (buffer[3] << 8) | buffer[4];
}

__declspec(dllexport) uint32_t Modbus_getValueUI32(HANDLE port, uint8_t block_addr, uint16_t reg, uint8_t* buffer, uint16_t size)
{
    read_port(port, buffer, size);

    char message[8] = {block_addr, MdBus_READ, reg >> 8, reg & 0xFF, 0x0, 0x2, 0x0, 0x0};
    
    uint16_t CRC = MODBUS_CRC(message, 6);
    message[6] = (CRC & 0xFF00) >> 8;
    message[7] = CRC & 0xFF;

    uint16_t in_size = 0;
    uint8_t count = 0;
    write_port(port, message, 8);
    
    while(!in_size)
    {
        count++;
        in_size = read_port(port, buffer, size);
        if(count == 0x10)
            return 0;
    }
    return (buffer[3] << 24) | (buffer[4] << 16) | (buffer[5] << 8) | buffer[6];
}

__declspec (dllexport) void Modbus_Write_SingleRegister (HANDLE port, uint8_t block_addr, uint16_t reg, uint16_t value)
{
    read_port(port, NULL, 0);

    char message[8] = {block_addr, MdBus_WRITE, reg >> 8, reg & 0xFF, value >> 8, value & 0xFF, 0, 0};
    uint16_t CRC = MODBUS_CRC(message, 6);
    message[6] = CRC >> 8;
    message[7] = CRC & 0xFF;

    write_port(port, message, 8);
    while(!read_port(port, message, 8))
    {}
}

__declspec(dllexport) uint16_t Modbus_DPS_Model(HANDLE port, uint8_t block_addr, uint8_t* buffer, uint8_t size)
{
    return Modbus_getValueUI16(port, block_addr, MODEL, buffer, size);
}

__declspec(dllexport) uint16_t Modbus_DPS_Version(HANDLE port, uint8_t block_addr, uint8_t* buffer, uint8_t size)
{
    return Modbus_getValueUI16(port, block_addr, VERSION, buffer, size);
}

__declspec(dllexport) BOOL Modbus_DPS_IsON(HANDLE port, uint8_t block_addr, uint8_t* buffer, uint8_t size)
{
    return Modbus_getValueUI16(port, block_addr, ONOFF, buffer, size);
}