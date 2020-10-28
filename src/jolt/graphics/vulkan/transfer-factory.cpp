#include <jolt/graphics/vulkan.hpp>
#include "transfer-factory.hpp"

namespace jolt::graphics::vulkan {
    UploadTransfer TransferFactory::build_upload_transfer() {
        UploadTransfer xfer{m_renderer, m_queue};

        for(auto const &desc : m_descriptors) { xfer.add_resource(desc); }

        m_descriptors.clear();

        return xfer;
    }

    DownloadTransfer TransferFactory::build_download_transfer() {
        DownloadTransfer xfer{m_renderer, m_queue};

        for(auto const &desc : m_descriptors) { xfer.add_resource(desc); }

        m_descriptors.clear();

        return xfer;
    }
} // namespace jolt::graphics::vulkan
