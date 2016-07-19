#include "usbprogrammer.h"

#define convertEndianess(A) ((((uint16_t)(A) & 0xff00) >> 8) | \
                             (((uint16_t)(A) & 0x00ff) << 8))

#define arr unsigned char[]

#define __packed __attribute__((__packed__))

struct RWPacket{
    uint16_t cmd;
    uint8_t  padding;
    uint16_t addr;
    uint16_t size;
    uint16_t data[505];
} __packed;

UsbProgrammer *UsbProgrammer::programmer = NULL;

UsbProgrammer::UsbProgrammer() :
    progInit(false)
{
    IsInitialized();
}

bool UsbProgrammer::ReadBlock(uint16_t addr, int size, uint16_t buffer[])
{
    if(!IsInitialized()) return false;

    RWPacket packet;
    packet.cmd = 0x100;
    packet.padding = 0;

    int readed = 0;
    unsigned short address = addr;

    do {

        int psize = 0x1F8;
        if(psize+readed > size)
            psize = size - readed;

        packet.addr = convertEndianess(address);
        packet.size = convertEndianess(psize);

        if(!usb.WriteData((unsigned char*)&packet,7)) return false;

        uint16_t tmp[3+psize];
        if(!usb.ReadData((unsigned char*)tmp,6+psize*2)) return false;

        for(int i=readed; i<readed+psize; i++)
            buffer[i] = convertEndianess(tmp[3+i-readed]);

        address += psize;
        readed += psize;

    } while (readed < size);


    return true;
}

bool UsbProgrammer::Read(uint16_t addr, uint16_t *data)
{
    if(!IsInitialized()) return false;

    RWPacket packet;
    packet.cmd = 0x100;
    packet.padding = 0;
    packet.size = 0x0100;
    packet.addr = convertEndianess(addr);

    if(!usb.WriteData((unsigned char*)&packet,7)) return false;

    uint16_t tmp[4];

    if(!usb.ReadData((unsigned char*)tmp,8)) return false;

    *data = convertEndianess(tmp[3]);

    return true;
}

bool UsbProgrammer::WriteBlock(uint16_t addr, int size, uint16_t buffer[])
{
    if(!IsInitialized()) return false;

    RWPacket packet;
    packet.cmd = 0x200;
    packet.padding = 0;

    int writed = 0;
    unsigned short address = addr;

    do {

        int psize = 0x1F8;
        if(psize+writed > size)
            psize = size - writed;

        packet.addr = convertEndianess(address);
        packet.size = convertEndianess(psize);

        for(int i=writed; i<writed+psize; i++)
            packet.data[i-writed] = convertEndianess(buffer[i]);

        if(!usb.WriteData((unsigned char*)&packet,7+psize*2)) return false;

        address += psize;
        writed += psize;

    } while (writed < size);


    return true;
}

bool UsbProgrammer::Write(uint16_t addr, uint16_t data)
{
    if(!IsInitialized()) return false;

    RWPacket packet = {
        0x200,
        0,
        convertEndianess(addr),
        0x0100,
        {convertEndianess(data)}
    };

    if(!usb.WriteData((unsigned char*)&packet,9)) return false;

    return true;
}

bool UsbProgrammer::SetTransferSpeed(uint16_t speed)
{
    unsigned char d[5];
    if(!usb.IsInitialized()) return false;

    uint8_t low = speed;
    uint8_t high = speed >> 8;
    d[0] = 0;
    d[1] = 3;
    d[2] = 0;
    d[3] = high;
    d[4] = low;
    return usb.WriteData(d,5);
}

bool UsbProgrammer::IsXAPStopped(void)
{
    unsigned char d[3];
    if(!IsInitialized()) return false;

    uint16_t data[2];
    d[0] = 0;
    d[1] = 4;
    d[2] = 0;
    usb.WriteData(d,3);
    usb.ReadData((unsigned char*)data,4);
    return data[1];
}

bool UsbProgrammer::IsInitialized(void)
{
    if(usb.IsInitialized()) {
        if(!progInit) {
            SetMode(true);
            SetTransferSpeed(0x189);
            ClearCmdBits();
            progInit = true;
        }
        return true;
    } else
        return false;
}

bool UsbProgrammer::SetMode(bool spi)
{
    unsigned char d[5];
    if(!usb.IsInitialized()) return false;

    uint8_t data = spi ? 0 : 0xff;
    d[0] = 0;
    d[1] = 9;
    d[2] = 0;
    d[3] = 0;
    d[4] = data;
    return usb.WriteData(d,5);
}

bool UsbProgrammer::ClearCmdBits()
{
    unsigned char d[7];
    if(!usb.IsInitialized()) return false;

    d[0] = 0;
    d[1] = 15;
    d[2] = 0;
    d[3] = 0;
    d[4] = 0;
    d[5] = 0;
    d[6] = 0;
    return usb.WriteData(d,7);
}

UsbProgrammer* UsbProgrammer::getProgrammer()
{
    if(programmer == NULL)
        programmer = new UsbProgrammer();

    return programmer;
}
