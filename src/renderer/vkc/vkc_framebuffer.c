VkFramebuffer *vkc_create_framebuffers(VkDevice device, VkRenderPass renderpass, struct vkc_swapchain *swapchain) {
    VkFramebuffer *fbs = VKC_MEMORY_ALLOC(sizeof(VkFramebuffer) * swapchain->image_view_count);

    for (u32 i = 0; i < swapchain->image_view_count; ++i) {
        VkImageView attachments[] = {
            swapchain->image_views[i]
        };

        VkFramebufferCreateInfo fb_info = {
            .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass      = renderpass,
            .attachmentCount = 1,
            .pAttachments    = attachments,
            .width           = swapchain->image_extent.width,
            .height          = swapchain->image_extent.height,
            .layers          = 1,
        };

        VK_CHECK(vkCreateFramebuffer(device, &fb_info, NULL, &fbs[i]),
                 "failed to create frame buffer");
    }

    return fbs;
}

void vkc_destroy_framebuffers(VkDevice device, VkFramebuffer *fbs, u32 fb_count) {
    for (u32 i = 0; i < fb_count; ++i) {
        vkDestroyFramebuffer(device, fbs[i], NULL);
    }

    VKC_MEMORY_FREE(fbs);
}
