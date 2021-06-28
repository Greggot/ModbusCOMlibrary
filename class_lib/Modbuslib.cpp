#include "Modbuslib.hpp"

Modbus::Modbus()
{}

uint16_t Modbus::MODBUS_CRC(uint8_t* str, uint8_t size)
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

int Modbus::write_port(uint8_t * buffer, size_t size)
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

HANDLE Modbus::open_serial_port(const char * device, uint32_t baud_rate)
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

SSIZE_T Modbus::read_port()
{
    DWORD received;
    if (!ReadFile(port, buffer, size, &received, NULL))
        return -1;

    return received;
}

///     PUBLIC CLASS METHODS       ///

uint16_t Modbus::Modbus_getValueUI16(uint16_t reg)
{
    
    /*PurgeComm(port, PURGE_RXCLEAR);
    read_port();
    
    std::cout << "TEMP BUFF: ";
    for(uint8_t i = 0; i < size; i++)
        std::cout << std::hex <<  (int)buffer[i] << ' ';
    std::cout << '\n';*/

    uint8_t upper = reg >> 8;
    uint8_t lower =  reg & 0xFF;
    uint8_t message[8] = {block_addr, MdBus_READ, upper, lower, 0x0, 0x1, 0x0, 0x0};
    uint16_t CRC = MODBUS_CRC(message, 6);
    message[6] = CRC >> 8;
    message[7] = CRC & 0xFF;

    std::cout << "MESSAGE: ";
    for(uint8_t i = 0; i < 8; i++)
        std::cout << std::hex <<  (int)message[i] << ' ';
    std::cout << '\n';

    uint16_t in_size = 0;
    uint8_t count = 0;
    write_port(message, 8);
    
    while(!in_size)
    {
        count++;

        /*std::cout << "TEMP BUFF: ";
        for(uint8_t i = 0; i < size; i++)
        {
            std::cout << std::hex <<  (int)buffer[i] << ' ';
            buffer[i] = 0;
        }
        std::cout << '\n';*/

        in_size = read_port();
        PurgeComm(port, PURGE_RXCLEAR);

        if(count == 0x10)
            return 0;
    }
    /*std::cout << "END BUFF: ";
    for(uint8_t i = 0; i < size; i++)
        std::cout << std::hex <<  (int)buffer[i] << ' ';
    std::cout << '\n';*/

    return (buffer[3] << 8) | buffer[4];
}

uint32_t Modbus::Modbus_getValueUI32(uint16_t reg)
{
    read_port();

    uint8_t upper = reg >> 8;
    uint8_t lower =  reg & 0xFF;
    uint8_t message[8] = {block_addr, MdBus_READ, upper, lower, 0x0, 0x2, 0x0, 0x0};
    
    uint16_t CRC = MODBUS_CRC(message, 6);
    message[6] = (CRC & 0xFF00) >> 8;
    message[7] = CRC & 0xFF;

    std::cout << "MESSAGE: ";
    for(uint8_t i = 0; i < 8; i++)
        std::cout << std::hex <<  (int)message[i] << ' ';
    std::cout << '\n';

    uint16_t in_size = 0;
    uint8_t count = 0;
    write_port(message, 8);
    
    while(!in_size)
    {
        count++;
        in_size = read_port();
        if(count == 0x10)
            return 0;
    }
    return (buffer[3] << 24) | (buffer[4] << 16) | (buffer[5] << 8) | buffer[6];
}

void Modbus::Modbus_Write_SingleRegister (uint16_t reg, uint16_t value)
{
    read_port();

    uint8_t upper = reg >> 8;
    uint8_t lower =  reg & 0xFF;
    uint8_t upper_val = value >> 8;
    uint8_t lower_val =  value & 0xFF;

    //std::cout << std::hex << (int)upper_val << " + " << (int)lower_val << " = " << value << '\n';

    uint8_t message[8] = {block_addr, MdBus_WRITE, upper, lower, upper_val, lower_val, 0, 0};
    uint16_t CRC = MODBUS_CRC(message, 6);
    message[6] = CRC >> 8;
    message[7] = CRC & 0xFF;

    uint8_t count = 0;
    write_port(message, 8);
    while(!read_port())
    {
        count++;
        if(count > 0x10)
            return;
    }
    std::cout << "ANS BUFF: ";
        for(uint8_t i = 0; i < size; i++)
        {
            std::cout << std::hex <<  (int)buffer[i] << ' ';
            buffer[i] = 0;
        }
        std::cout << '\n';
    std::cout << '\n';
    Sleep(5);
}


///                         ---ModBusDeviceDSP---                               ///


uint16_t ModbusDeviceDPS::Model()
{
    return Modbus_getValueUI16(MODEL);
}

uint16_t ModbusDeviceDPS::Version()
{
    return Modbus_getValueUI16(VERSION);
}

uint16_t ModbusDeviceDPS::Uin()
{
    return Modbus_getValueUI16(UIN);
}

uint16_t ModbusDeviceDPS::Uset()
{
    return Modbus_getValueUI16(USET);
}
uint16_t ModbusDeviceDPS::Iset()
{
    return Modbus_getValueUI16(ISET);
}

uint16_t ModbusDeviceDPS::Uout()
{
    return Modbus_getValueUI16(UOUT);
}
uint16_t ModbusDeviceDPS::Iout()
{
    return Modbus_getValueUI16(IOUT);
}

uint32_t ModbusDeviceDPS::getVA()
{
    return Modbus_getValueUI32(UOUT);
}

void ModbusDeviceDPS::Uset(uint16_t voltage)
{
    Modbus_Write_SingleRegister(USET, voltage);
}
void ModbusDeviceDPS::Iset(uint16_t amperage)
{
    Modbus_Write_SingleRegister(ISET, amperage);
}

bool ModbusDeviceDPS::IsON()
{
    return Modbus_getValueUI16(ONOFF);
}

void ModbusDeviceDPS::ON()
{
    Modbus_Write_SingleRegister(ONOFF, 1);
}

void ModbusDeviceDPS::OFF()
{
    Modbus_Write_SingleRegister(ONOFF, 0);
}

ModbusDeviceDPS::ModbusDeviceDPS(uint8_t block_addr, uint8_t COM, uint32_t baudrate)
{
    std::cout << "This block\'s address is " << (int)block_addr << '\n';
    this->block_addr = block_addr;
    char COM_addr[9] = {'\\', '\\', '.', '\\', 'C', 'O', 'M', COM + 0x30, 0};

    port = open_serial_port(COM_addr, baudrate);
    for(uint8_t i = 0; i < size; i++)
        buffer[i] = 0;
}

ModbusDeviceDPS::ModbusDeviceDPS(uint8_t block_addr, HANDLE port)
{
    std::cout << "This block\'s address is " << (int)block_addr << '\n';
    this->block_addr = block_addr;
    this->port = port;

    for(uint8_t i = 0; i < size; i++)
        buffer[i] = 0;
}

HANDLE ModbusDeviceDPS::getPort()
{
    return port;
}