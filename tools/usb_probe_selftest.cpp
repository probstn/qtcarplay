#include <libusb.h>

#include <array>
#include <iomanip>
#include <iostream>

namespace {
constexpr uint16_t VendorId = 0x1314;
constexpr std::array<uint16_t, 2> ProductIds = {0x1520, 0x1521};

bool isKnownProduct(uint16_t productId)
{
    for (const uint16_t known : ProductIds) {
        if (known == productId)
            return true;
    }
    return false;
}
}

int main()
{
    libusb_context *context = nullptr;
    int result = libusb_init(&context);
    if (result != LIBUSB_SUCCESS) {
        std::cerr << "libusb_init failed: " << libusb_error_name(result) << '\n';
        return 1;
    }

    libusb_device **devices = nullptr;
    const ssize_t deviceCount = libusb_get_device_list(context, &devices);
    if (deviceCount < 0) {
        std::cerr << "Device list failed: " << libusb_error_name(static_cast<int>(deviceCount)) << '\n';
        libusb_exit(context);
        return 2;
    }

    libusb_device_handle *handle = nullptr;
    libusb_device_descriptor descriptor {};
    bool foundDescriptor = false;

    for (ssize_t i = 0; i < deviceCount; ++i) {
        libusb_device_descriptor candidate {};
        result = libusb_get_device_descriptor(devices[i], &candidate);
        if (result != LIBUSB_SUCCESS)
            continue;

        if (candidate.idVendor != VendorId || !isKnownProduct(candidate.idProduct))
            continue;

        foundDescriptor = true;
        descriptor = candidate;

        std::cout << "Matching dongle enumerated: VID 0x" << std::hex << std::setw(4) << std::setfill('0')
                  << candidate.idVendor << " PID 0x" << std::setw(4) << candidate.idProduct
                  << " bus " << std::dec << static_cast<int>(libusb_get_bus_number(devices[i]))
                  << " address " << static_cast<int>(libusb_get_device_address(devices[i])) << '\n';

        result = libusb_open(devices[i], &handle);
        if (result == LIBUSB_SUCCESS && handle)
            break;

        std::cerr << "Opening matching dongle failed: " << libusb_error_name(result) << '\n';
    }

    libusb_free_device_list(devices, 1);

    if (!handle) {
        std::cerr << (foundDescriptor ? "Dongle found but could not be opened" : "Dongle not found")
                  << ": VID 0x" << std::hex << VendorId << " PID 0x1520/0x1521\n";
        libusb_exit(context);
        return foundDescriptor ? 3 : 2;
    }

    libusb_device *device = libusb_get_device(handle);
    result = libusb_get_device_descriptor(device, &descriptor);
    if (result != LIBUSB_SUCCESS) {
        std::cerr << "Descriptor read failed: " << libusb_error_name(result) << '\n';
        libusb_close(handle);
        libusb_exit(context);
        return 4;
    }

    libusb_config_descriptor *config = nullptr;
    result = libusb_get_active_config_descriptor(device, &config);
    if (result != LIBUSB_SUCCESS) {
        std::cerr << "Active config read failed: " << libusb_error_name(result) << '\n';
        libusb_close(handle);
        libusb_exit(context);
        return 5;
    }

    uint8_t endpointIn = 0;
    uint8_t endpointOut = 0;
    if (config->bNumInterfaces > 0 && config->interface[0].num_altsetting > 0) {
        const libusb_interface_descriptor &interface = config->interface[0].altsetting[0];
        for (int i = 0; i < interface.bNumEndpoints; ++i) {
            const libusb_endpoint_descriptor &endpoint = interface.endpoint[i];
            if ((endpoint.bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_IN)
                endpointIn = endpoint.bEndpointAddress;
            else
                endpointOut = endpoint.bEndpointAddress;
        }
    }

    std::cout << "Dongle found: VID 0x" << std::hex << std::setw(4) << std::setfill('0') << descriptor.idVendor
              << " PID 0x" << std::setw(4) << descriptor.idProduct
              << " USB " << std::dec << (descriptor.bcdUSB >> 8) << "." << (descriptor.bcdUSB & 0xff)
              << " IN 0x" << std::hex << static_cast<int>(endpointIn)
              << " OUT 0x" << static_cast<int>(endpointOut) << '\n';

    libusb_free_config_descriptor(config);
    libusb_close(handle);
    libusb_exit(context);

    return endpointIn && endpointOut ? 0 : 6;
}
