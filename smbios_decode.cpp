/*
 * Copyright 2019 Bruno Ribeiro
 * https://github.com/brunexgeek/smbios-parser
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <sys/stat.h>
#include <iomanip>
#include <qjsonobject.h>
#include <qjsondocument.h>
#include <qtextstream.h>
#include "smbios.h"
#include "smbios_decode.h"

#ifdef _WIN32

#include <Windows.h>

bool getDMI( std::vector<uint8_t> &buffer )
{
    const BYTE byteSignature[] = { 'B', 'M', 'S', 'R' };
    const DWORD signature = *((DWORD*)byteSignature);

    // get the size of SMBIOS table
    DWORD size = GetSystemFirmwareTable(signature, 0, NULL, 0);
    if (size == 0) return false;
    buffer.resize(size, 0);
    // retrieve the SMBIOS table

    if (size != GetSystemFirmwareTable(signature, 0, buffer.data(), size))
    {
        buffer.clear();
        return false;
    }

    return true;
}

#else

bool getDMI( const std::string &path, std::vector<uint8_t> &buffer )
{
    std::ifstream input;
    std::string fileName;

    // get the SMBIOS structures size
    fileName = path + "/DMI";
    struct stat info;
    if (stat(fileName.c_str(), &info) != 0) return false;
    buffer.resize(info.st_size + 32);

    // read SMBIOS structures
    input.open(fileName.c_str(), std::ios_base::binary);
    if (!input.good()) return false;
    input.read((char*) buffer.data() + 32, info.st_size);
    input.close();

    // read SMBIOS entry point
    fileName = path + "/smbios_entry_point";
    input.open(fileName.c_str(), std::ios_base::binary);
    if (!input.good()) return false;
    input.read((char*) buffer.data(), 32);
    input.close();

    return true;
}

#endif

bool printSMBIOS(
    smbios::Parser &parser,
    QTextStream &output)
{
	QJsonObject biosObject;
    int version = parser.version();
    const smbios::Entry *entry = NULL;
    while (true)
    {
        entry = parser.next();
        if (entry == NULL) break;
        /*output << "Handle 0x" << std::hex << std::setw(4) << std::setfill('0') << (int) entry->handle << std::dec
            << ", DMI Type " << (int) entry->type << ", " << (int) entry->length << " bytes\n";*/

        if (entry->type == DMI_TYPE_BIOS)
        {
            if (version >= smbios::SMBIOS_2_0)
            {
                output << "[bios] vendor:" << entry->data.bios.Vendor << '\n';
                output << "[bios] version:" << entry->data.bios.BIOSVersion << '\n';
                output << "[bios] starting_segment:" << std::hex << (int) entry->data.bios.BIOSStartingSegment << std::dec << '\n';
                output << "[bios] release_date:" << entry->data.bios.BIOSReleaseDate << '\n';
                output << "[bios] rom_size:" << (((int) entry->data.bios.BIOSROMSize + 1) * 64) << " KiB \n";
            }
            if (version >= smbios::SMBIOS_2_4)
            {
                output << "[bios] system_bios_major_release:" << (int) entry->data.bios.SystemBIOSMajorRelease  << '\n';
                output << "[bios] system_bios_minor_release:" << (int) entry->data.bios.SystemBIOSMinorRelease  << '\n';
                output << "[bios] embedded_firmware_major_release:" << (int) entry->data.bios.EmbeddedFirmwareMajorRelease  << '\n';
                output << "[bios] embedded_firmware_minor_release:" << (int) entry->data.bios.EmbeddedFirmwareMinorRelease  << '\n';
            }
            output << '\n';
        }
        else
        if (entry->type == DMI_TYPE_SYSINFO)
        {
            if (version >= smbios::SMBIOS_2_0)
            {
                output << "[sysinfo] manufacturer:" << entry->data.sysinfo.Manufacturer << '\n';
                output << "[sysinfo] product_name:" << entry->data.sysinfo.ProductName << '\n';
                output << "[sysinfo] version:" << entry->data.sysinfo.Version << '\n';
                output << "[sysinfo] serial_number:" << entry->data.sysinfo.SerialNumber << '\n';
            }
            if (version >= smbios::SMBIOS_2_1)
            {
                //output << "[sysinfo] uuid:";
                /*for (size_t i = 0; i < 16; ++i)
                    output << std::hex << std::setw(2) << std::setfill('0') << (int) entry->data.sysinfo.UUID[i]  << ' ';
                output << '\n' << std::dec;*/
            }
            if (version >= smbios::SMBIOS_2_4)
            {
                output << "[sysinfo] sku_number:" << entry->data.sysinfo.SKUNumber << '\n';
                output << "[sysinfo] family:" << entry->data.sysinfo.Family << '\n';
            }
            output << '\n';
        }
        else
        if (entry->type == DMI_TYPE_BASEBOARD)
        {
            if (version >= smbios::SMBIOS_2_0)
            {
                output << "[baseboard] manufacturer:" << entry->data.baseboard.Manufacturer << '\n';
                output << "[baseboard] product:" << entry->data.baseboard.Product << '\n';
                output << "[baseboard] version:" << entry->data.baseboard.Version << '\n';
                output << "[baseboard] serial_number:" << entry->data.baseboard.SerialNumber << '\n';
                output << "[baseboard] asset_tag":" << entry->data.baseboard.AssetTag << '\n';
                output << "[baseboard] location_in_chassis:" << entry->data.baseboard.LocationInChassis << '\n';
                output << "[baseboard] chassis_handle:" << entry->data.baseboard.ChassisHandle << '\n';
                output << "[baseboard] board_type:" << (int) entry->data.baseboard.BoardType << '\n';
            }
            output << '\n';
        }
        else
        if (entry->type == DMI_TYPE_SYSENCLOSURE)
        {
            if (version >= smbios::SMBIOS_2_0)
            {
                output << "[sysenclosure] manufacturer:" << entry->data.sysenclosure.Manufacturer << '\n';
                output << "[sysenclosure] version:" << entry->data.sysenclosure.Version << '\n';
                output << "[sysenclosure] serial_number:" << entry->data.sysenclosure.SerialNumber << '\n';
                output << "[sysenclosure] asset_tag:" << entry->data.sysenclosure.AssetTag << "\n";
            }
            if (version >= smbios::SMBIOS_2_3)
            {
                output << "[sysenclosure] contained_count:" << (int) entry->data.sysenclosure.ContainedElementCount << '\n';
                output << "[sysenclosure] contained_length:" << (int) entry->data.sysenclosure.ContainedElementRecordLength << '\n';
            }
            if (version >= smbios::SMBIOS_2_7)
            {
                output << "[sysenclosure] sku_number:" << entry->data.sysenclosure.SKUNumber << '\n';
            }
            output << '\n';
        }
        else
        if (entry->type == DMI_TYPE_PROCESSOR)
        {
            if (version >= smbios::SMBIOS_2_0)
            {
                output << "[processor] socket_designation:" << entry->data.processor.SocketDesignation << '\n';
                output << "[processor] processor_family:" << (int) entry->data.processor.ProcessorFamily << '\n';
                output << "[processor] manufacturer:" << entry->data.processor.ProcessorManufacturer << '\n';
                output << "[processor] version:" << entry->data.processor.ProcessorVersion << '\n';
                //output << "[processor] processor_id:";
                /*for (size_t i = 0; i < 8; ++i)
                    output << std::hex << std::setw(2) << std::setfill('0') << (int) entry->data.processor.ProcessorID[i] << ' ';
                output << std::dec << '\n';*/
            }
            if (version >= smbios::SMBIOS_2_5)
            {
                output << "[processor] core_count:" << (int) entry->data.processor.CoreCount << '\n';
                output << "[processor] core_enabled:" << (int) entry->data.processor.CoreEnabled << '\n';
                output << "[processor] thread_count:" << (int) entry->data.processor.ThreadCount << '\n';
            }
            if (version >= smbios::SMBIOS_2_6)
            {
                output << "[processor] processor_family_2:" << entry->data.processor.ProcessorFamily2 << '\n';
            }
            output << '\n';
        }
        else
        if (entry->type == DMI_TYPE_SYSSLOT)
        {
            if (version >= smbios::SMBIOS_2_0)
            {
                output << "[sysslot] slot_designation:" << entry->data.sysslot.SlotDesignation << '\n';
                output << "[sysslot] slot_type:" << (int) entry->data.sysslot.SlotType << '\n';
                output << "[sysslot] slot_data_bus_width:" << (int) entry->data.sysslot.SlotDataBusWidth << '\n';
                output << "[sysslot] slot_id:" << (int) entry->data.sysslot.SlotID << '\n';
            }
            if (version >= smbios::SMBIOS_2_6)
            {
                output << "[sysslot] segment_group_number:" << entry->data.sysslot.SegmentGroupNumber << '\n';
                output << "[sysslot] bus_number:" << (int) entry->data.sysslot.BusNumber << '\n';
            }
            output << '\n';
        }
        else
        if (entry->type == DMI_TYPE_PHYSMEM)
        {
            if (version >= smbios::SMBIOS_2_1)
            {
                output << "[physmem] use:" << std::hex << (int) entry->data.physmem.Use << std::dec << '\n';
                output << "[physmem] number_devices:" << entry->data.physmem.NumberDevices << '\n';
                output << "[physmem] maximum_capacity:" << entry->data.physmem.MaximumCapacity << " KiB\n";
                output << "[physmem] ext_maximum_capacity:" << entry->data.physmem.ExtendedMaximumCapacity << " KiB\n";
            }
            output << '\n';
        }
        else
        if (entry->type == DMI_TYPE_MEMORY)
        {
            if (version >= smbios::SMBIOS_2_1)
            {
                output << "[memory] device_locator:" << entry->data.memory.DeviceLocator << '\n';
                output << "[memory] bank_locator:" << entry->data.memory.BankLocator << '\n';
            }
            if (version >= smbios::SMBIOS_2_3)
            {
                output << "[memory] speed:" << entry->data.memory.Speed << " MHz\n";
                output << "[memory] manufacturer:" << entry->data.memory.Manufacturer << '\n';
                output << "[memory] serial_number:" << entry->data.memory.SerialNumber << '\n';
                output << "[memory] asset_tag_number:" << entry->data.memory.AssetTagNumber << '\n';
                output << "[memory] part_number" << entry->data.memory.PartNumber << '\n';
                output << "[memory] size:" << entry->data.memory.Size << " MiB\n";
                output << "[memory] extended_size:" << entry->data.memory.ExtendedSize << " MiB\n";
            }
            if (version >= smbios::SMBIOS_2_7)
            {
                output << "[memory] configured_clock_speed:" << entry->data.memory.ConfiguredClockSpeed << " MHz\n";
            }
            output << '\n';
        }
        else
        if (entry->type == DMI_TYPE_OEMSTRINGS)
        {
            if (version >= smbios::SMBIOS_2_0)
            {
                output << "[oemstrings] count:" << (int) entry->data.oemstrings.Count << '\n';
                const char *ptr = entry->data.oemstrings.Values;
                int c = entry->data.oemstrings.Count;
                while (ptr != nullptr && *ptr != 0 && c > 0)
                {
                    output << "         [" << ptr << "]" << '\n';
                    while (*ptr != 0) ++ptr;
                    ++ptr;
                }
            }
            output << '\n';
        }

    }

    return true;
}

/*int main(int argc, char ** argv)
{
    std::vector<uint8_t> buffer;
    bool result = false;

    #ifdef _WIN32

    result = getDMI(buffer);

    #else

    const char *path = "/sys/firmware/dmi/tables";
    if (argc == 2) path = argv[1];
    std::cerr << "Using SMBIOS tables from " << path << std::endl;
    result = getDMI(path, buffer);

    #endif

    if (!result)
    {
        std::cerr << "Unable to open SMBIOS tables" << std::endl;
        return 1;
    }

    smbios::Parser parser(buffer.data(), buffer.size());
    if (parser.valid())
        printSMBIOS(parser, std::cout);
    else
        std::cerr << "Invalid SMBIOS data" << std::endl;

    return 0;
}*/
