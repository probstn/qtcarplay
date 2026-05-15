#include <libusb.h>

#include <iomanip>
#include <iostream>

namespace {
constexpr uint16_t VendorId = 0x1314;
constexpr uint16_t ProductId = 0x1521;
}

int main()
{
    libusb_context *context = nullptr;
    int result = libusb_init(&context);
    if (result != LIBUSB_SUCCESS) {
        std::cerr << "libusb_init failed: " << libusb_error_name(result) << '\n';
        return 1;
    }

    libusb_device_handle *handle = libusb_open_device_with_vid_pid(context, VendorId, ProductId);
    if (!handle) {
        std::cerr << "Dongle not found: VID 0x" << std::hex << VendorId
                  << " PID 0x" << ProductId << '\n';
        libusb_exit(context);
        return 2;
    }

    libusb_device *device = libusb_get_device(handle);
    libusb_device_descriptor descriptor {};
    result = libusb_get_device_descriptor(device, &descriptor);
    if (result != LIBUSB_SUCCESS) {
        std::cerr << "Descriptor read failed: " << libusb_error_name(result) << '\n';
        libusb_close(handle);
        libusb_exit(context);
        return 3;
    }

    libusb_config_descriptor *config = nullptr;
    result = libusb_get_active_config_descriptor(device, &config);
    if (result != LIBUSB_SUCCESS) {
        std::cerr << "Active config read failed: " << libusb_error_name(result) << '\n';
        libusb_close(handle);
        libusb_exit(context);
        return 4;
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

    return endpointIn && endpointOut ? 0 : 5;
}
