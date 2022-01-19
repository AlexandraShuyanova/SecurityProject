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
                output << "BIOS_vendor:" << entry->data.bios.Vendor << '\n';
                output << "BIOS_version:" << entry->data.bios.BIOSVersion << '\n';
                output << "BIOS_starting_segment:" << std::hex << (int) entry->data.bios.BIOSStartingSegment << std::dec << '\n';
                output << "BIOS_release_date:" << entry->data.bios.BIOSReleaseDate << '\n';
                output << "BIOS_rom_size:" << (((int) entry->data.bios.BIOSROMSize + 1) * 64) << " KiB \n";
            }
            if (version >= smbios::SMBIOS_2_4)
            {
                output << "BIOS_system_bios_major_release:" << (int) entry->data.bios.SystemBIOSMajorRelease  << '\n';
                output << "BIOS_system_bios_minor_release:" << (int) entry->data.bios.SystemBIOSMinorRelease  << '\n';
                output << "BIOS_embedded_firmware_major_release:" << (int) entry->data.bios.EmbeddedFirmwareMajorRelease  << '\n';
                output << "BIOS_embedded_firmware_minor_release:" << (int) entry->data.bios.EmbeddedFirmwareMinorRelease  << '\n';
            }
            output << '\n';
        }
        else
        if (entry->type == DMI_TYPE_SYSINFO)
        {
            if (version >= smbios::SMBIOS_2_0)
            {
                output << "SYSINFO_manufacturer:" << entry->data.sysinfo.Manufacturer << '\n';
                output << "SYSINFO_product_name:" << entry->data.sysinfo.ProductName << '\n';
                output << "SYSINFO_version:" << entry->data.sysinfo.Version << '\n';
                output << "SYSINFO_serial_number:" << entry->data.sysinfo.SerialNumber << '\n';
            }
            if (version >= smbios::SMBIOS_2_1)
            {
                //output << "SYSINFO_uuid:";
                /*for (size_t i = 0; i < 16; ++i)
                    output << std::hex << std::setw(2) << std::setfill('0') << (int) entry->data.sysinfo.UUID[i]  << ' ';
                output << '\n' << std::dec;*/
            }
            if (version >= smbios::SMBIOS_2_4)
            {
                output << "SYSINFO_sku_number:" << entry->data.sysinfo.SKUNumber << '\n';
                output << "SYSINFO_family:" << entry->data.sysinfo.Family << '\n';
            }
            output << '\n';
        }
        else
        if (entry->type == DMI_TYPE_BASEBOARD)
        {
            if (version >= smbios::SMBIOS_2_0)
            {
                output << "BASEBOARD_manufacturer:" << entry->data.baseboard.Manufacturer << '\n';
                output << "BASEBOARD_product:" << entry->data.baseboard.Product << '\n';
                output << "BASEBOARD_version:" << entry->data.baseboard.Version << '\n';
                output << "BASEBOARD_serial_number:" << entry->data.baseboard.SerialNumber << '\n';
                output << "BASEBOARD_asset_tag":" << entry->data.baseboard.AssetTag << '\n';
                output << "BASEBOARD_location_in_chassis:" << entry->data.baseboard.LocationInChassis << '\n';
                output << "BASEBOARD_chassis_handle:" << entry->data.baseboard.ChassisHandle << '\n';
                output << "BASEBOARD_board_type:" << (int) entry->data.baseboard.BoardType << '\n';
            }
            output << '\n';
        }
        else
        if (entry->type == DMI_TYPE_SYSENCLOSURE)
        {
            if (version >= smbios::SMBIOS_2_0)
            {
                output << "SYSENCLOSURE_manufacturer:" << entry->data.sysenclosure.Manufacturer << '\n';
                output << "SYSENCLOSURE_version:" << entry->data.sysenclosure.Version << '\n';
                output << "SYSENCLOSURE_serial_number:" << entry->data.sysenclosure.SerialNumber << '\n';
                output << "SYSENCLOSURE_asset_tag:" << entry->data.sysenclosure.AssetTag << "\n";
            }
            if (version >= smbios::SMBIOS_2_3)
            {
                output << "SYSENCLOSURE_contained_count:" << (int) entry->data.sysenclosure.ContainedElementCount << '\n';
                output << "SYSENCLOSURE_contained_length:" << (int) entry->data.sysenclosure.ContainedElementRecordLength << '\n';
            }
            if (version >= smbios::SMBIOS_2_7)
            {
                output << "SYSENCLOSURE_sku_number:" << entry->data.sysenclosure.SKUNumber << '\n';
            }
            output << '\n';
        }
        else
        if (entry->type == DMI_TYPE_PROCESSOR)
        {
            if (version >= smbios::SMBIOS_2_0)
            {
                output << "PROCESSOR_socket_designation:" << entry->data.processor.SocketDesignation << '\n';
                output << "PROCESSOR_processor_family:" << (int) entry->data.processor.ProcessorFamily << '\n';
                output << "PROCESSOR_manufacturer:" << entry->data.processor.ProcessorManufacturer << '\n';
                output << "PROCESSOR_version:" << entry->data.processor.ProcessorVersion << '\n';
                //output << "PROCESSOR_processor_id:";
                /*for (size_t i = 0; i < 8; ++i)
                    output << std::hex << std::setw(2) << std::setfill('0') << (int) entry->data.processor.ProcessorID[i] << ' ';
                output << std::dec << '\n';*/
            }
            if (version >= smbios::SMBIOS_2_5)
            {
                output << "PROCESSOR_core_count:" << (int) entry->data.processor.CoreCount << '\n';
                output << "PROCESSOR_core_enabled:" << (int) entry->data.processor.CoreEnabled << '\n';
                output << "PROCESSOR_thread_count:" << (int) entry->data.processor.ThreadCount << '\n';
            }
            if (version >= smbios::SMBIOS_2_6)
            {
                output << "PROCESSOR_processor_family_2:" << entry->data.processor.ProcessorFamily2 << '\n';
            }
            output << '\n';
        }
        else
        if (entry->type == DMI_TYPE_SYSSLOT)
        {
            if (version >= smbios::SMBIOS_2_0)
            {
                output << "SYSSLOT_slot_designation:" << entry->data.sysslot.SlotDesignation << '\n';
                output << "SYSSLOT_slot_type:" << (int) entry->data.sysslot.SlotType << '\n';
                output << "SYSSLOT_slot_data_bus_width:" << (int) entry->data.sysslot.SlotDataBusWidth << '\n';
                output << "SYSSLOT_slot_id:" << (int) entry->data.sysslot.SlotID << '\n';
            }
            if (version >= smbios::SMBIOS_2_6)
            {
                output << "SYSSLOT_segment_group_number:" << entry->data.sysslot.SegmentGroupNumber << '\n';
                output << "SYSSLOT_bus_number:" << (int) entry->data.sysslot.BusNumber << '\n';
            }
            output << '\n';
        }
        else
        if (entry->type == DMI_TYPE_PHYSMEM)
        {
            if (version >= smbios::SMBIOS_2_1)
            {
                output << "PHYSMEM_use:" << std::hex << (int) entry->data.physmem.Use << std::dec << '\n';
                output << "PHYSMEM_number_devices:" << entry->data.physmem.NumberDevices << '\n';
                output << "PHYSMEM_maximum_capacity:" << entry->data.physmem.MaximumCapacity << " KiB\n";
                output << "PHYSMEM_ext_maximum_capacity:" << entry->data.physmem.ExtendedMaximumCapacity << " KiB\n";
            }
            output << '\n';
        }
        else
        if (entry->type == DMI_TYPE_MEMORY)
        {
            if (version >= smbios::SMBIOS_2_1)
            {
                output << "MEMORY_device_locator:" << entry->data.memory.DeviceLocator << '\n';
                output << "MEMORY_bank_locator:" << entry->data.memory.BankLocator << '\n';
            }
            if (version >= smbios::SMBIOS_2_3)
            {
                output << "MEMORY_speed:" << entry->data.memory.Speed << " MHz\n";
                output << "MEMORY_manufacturer:" << entry->data.memory.Manufacturer << '\n';
                output << "MEMORY_serial_number:" << entry->data.memory.SerialNumber << '\n';
                output << "MEMORY_asset_tag_number:" << entry->data.memory.AssetTagNumber << '\n';
                output << "MEMORY_part_number" << entry->data.memory.PartNumber << '\n';
                output << "MEMORY_size:" << entry->data.memory.Size << " MiB\n";
                output << "MEMORY_extended_size:" << entry->data.memory.ExtendedSize << " MiB\n";
            }
            if (version >= smbios::SMBIOS_2_7)
            {
                output << "MEMORY_configured_clock_speed:" << entry->data.memory.ConfiguredClockSpeed << " MHz\n";
            }
            output << '\n';
        }
        else
        if (entry->type == DMI_TYPE_OEMSTRINGS)
        {
            if (version >= smbios::SMBIOS_2_0)
            {
                output << "OEMSTRINGS_count:" << (int) entry->data.oemstrings.Count << '\n';
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
