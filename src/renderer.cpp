#include "renderer.h"

#include <glm/ext.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include "gfx/common.h"
#include "gfx/gfx.h"
#include "gfx/vk_util.h"
#include "pipeline.h"
#include "simulation.h"

namespace vfs {

namespace {

void DrawCircleFill(gfx::CPUMesh& mesh, const glm::vec3& center, float radius, int steps) {
    float cx = center.x;
    float cy = center.y;

    for (int i = 0; i < steps; i++) {
        float angle0 = (float)i * 2 * M_PI / (float)steps;
        float angle1 = (float)(i + 1) * 2 * M_PI / (float)steps;

        auto p0 = glm::vec3(cx, cy, 0.0f);
        auto p1 = glm::vec3(cx + radius * sinf(angle0), cy + radius * cosf(angle0), 0.0f);
        auto p2 = glm::vec3(cx + radius * sinf(angle1), cy + radius * cosf(angle1), 0.0f);

        const unsigned int n = (unsigned int)mesh.vertices.size();
        mesh.indices.insert(mesh.indices.end(), {n + 0, n + 2, n + 1});
        mesh.vertices.insert(mesh.vertices.end(),
                             {{.pos = p0, .uv = {}}, {.pos = p1, .uv = {}}, {.pos = p2, .uv = {}}});
    }
}

void DrawSquare(gfx::CPUMesh& mesh, const glm::vec3& center, float side, const glm::vec4& color) {
    const unsigned int n = (unsigned int)mesh.vertices.size();

    float hs = 0.5 * side;

    auto ll = glm::vec3(center.x - hs, center.y - hs, 0.0f);
    auto lr = glm::vec3(center.x + hs, center.y - hs, 0.0f);
    auto ur = glm::vec3(center.x + hs, center.y + hs, 0.0f);
    auto ul = glm::vec3(center.x - hs, center.y + hs, 0.0f);

    mesh.indices.insert(mesh.indices.end(), {n + 0, n + 2, n + 1, n + 0, n + 3, n + 2});
    mesh.vertices.push_back({.pos = ll, .uv = {0.0f, 0.0f}});
    mesh.vertices.push_back({.pos = lr, .uv = {1.0f, 0.0f}});
    mesh.vertices.push_back({.pos = ur, .uv = {1.0f, 1.0f}});
    mesh.vertices.push_back({.pos = ul, .uv = {0.0f, 1.0f}});
}

gfx::Image CreateDrawImage(const gfx::Device& gfx, u32 w, u32 h) {
    VkExtent3D extent = {.width = (uint32_t)w, .height = (uint32_t)h, .depth = 1};
    VkFormat format = VK_FORMAT_B8G8R8A8_UNORM;

    VkImageUsageFlags usage{0};
    usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    usage |= VK_IMAGE_USAGE_STORAGE_BIT;

    bool use_mipmaps = false;

    return gfx.CreateImage(extent, format, usage, use_mipmaps);
}

gfx::Image CreateDepthImage(const gfx::Device& gfx, u32 w, u32 h) {
    VkExtent3D extent = {.width = (uint32_t)w, .height = (uint32_t)h, .depth = 1};
    VkFormat format = VK_FORMAT_D32_SFLOAT;

    VkImageUsageFlags usage{0};
    usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    return gfx.CreateImage(extent, format, usage, false);
}

}  // namespace

void SimulationRenderer2D::Init(const gfx::Device& gfx,
                                const Simulation2D& simulation,
                                int w,
                                int h) {
    draw_img = CreateDrawImage(gfx, w, h);
    depth_img = CreateDepthImage(gfx, w, h);

    // TODO: Create other important images:
    //      1. Post fx image
    //      ...

    clear_color = {0.0f, 0.0f, 0.0f, 1.0f};

    sprite_pipeline.Init(gfx.GetCoreCtx(), draw_img.format);

    gfx::CPUMesh mesh;
    DrawSquare(mesh, glm::vec3(0.0f), 0.2f, glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
    particle_mesh = gfx::UploadMesh(gfx, mesh);

    box_pipeline.Init(gfx.GetCoreCtx(), draw_img.format, depth_img.format);
}

void SimulationRenderer2D::Draw(gfx::Device& gfx,
                                VkCommandBuffer cmd,
                                const Simulation2D& simulation,
                                u32 current_frame,
                                const glm::mat4 view_proj) {
    auto bbox = simulation.GetBoundingBox();
    box_transform.SetScale(glm::vec3(bbox.size, 1.0f));
    box_transform.SetPosition(glm::vec3(bbox.pos, 0.0f));

    const auto& buffers = simulation.GetFrameData(current_frame);
    auto pos_buffer = buffers.position_buffer.device_addr;
    auto vel_buffer = buffers.velocity_buffer.device_addr;

    auto color_attachment = vk::util::RenderingAttachmentInfo(
        draw_img.view, NULL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.clearValue.color = {
        clear_color.r,
        clear_color.g,
        clear_color.b,
        clear_color.a,
    };

    // auto depth_attachment = vk::util::DepthRenderingAttachmentInfo(
    //     depth_img.view, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    auto render_info =
        vk::util::RenderingInfo(gfx.GetSwapchainExtent(), &color_attachment, nullptr);

    vkCmdBeginRendering(cmd, &render_info);

    const auto pc = ParticleDrawPipeline::PushConstants{
        .matrix = view_proj * transform.Matrix(),
        .positions = pos_buffer,
        .velocities = vel_buffer,
    };

    sprite_pipeline.Draw(cmd, gfx, draw_img, particle_mesh, pc,
                         simulation.GetParameters().n_particles);

    const auto pc_box = BoxDrawPipeline::PushConstants{
        .color = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f),
        .matrix = view_proj * transform.Matrix() * box_transform.Matrix(),
    };
    box_pipeline.Draw(cmd, gfx, draw_img, pc_box);

    vkCmdEndRendering(cmd);
}

void SimulationRenderer2D::Clear(const gfx::Device& gfx) {
    gfx::DestroyMesh(gfx, particle_mesh);
    gfx.DestroyImage(draw_img);
    gfx.DestroyImage(depth_img);
    sprite_pipeline.Clear(gfx.GetCoreCtx());
    box_pipeline.Clear(gfx.GetCoreCtx());
}

void SimulationRenderer3D::Init(const gfx::Device& gfx,
                                const Simulation3D& simulation,
                                int w,
                                int h) {
    draw_img = CreateDrawImage(gfx, w, h);
    depth_img = CreateDepthImage(gfx, w, h);

    clear_color = {0.0f, 0.0f, 0.0f, 1.0f};

    box_pipeline.Init(gfx.GetCoreCtx(), draw_img.format, depth_img.format, true);
    particles_pipeline.Init(gfx.GetCoreCtx(), draw_img.format, depth_img.format);

    gfx::CPUMesh mesh;
    // DrawSquare(mesh, glm::vec3(0.0f), 0.2f, glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
    DrawCircleFill(mesh, glm::vec3(0.0f), 0.1f, 3);
    particle_mesh = gfx::UploadMesh(gfx, mesh);
}

void SimulationRenderer3D::Draw(gfx::Device& gfx,
                                VkCommandBuffer cmd,
                                const Simulation3D& simulation,
                                u32 current_frame,
                                const Camera& camera) {
    auto color_attachment = vk::util::RenderingAttachmentInfo(
        draw_img.view, NULL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.clearValue.color = {
        clear_color.r,
        clear_color.g,
        clear_color.b,
        clear_color.a,
    };

    vk::util::TransitionImage(cmd, depth_img.image, VK_IMAGE_LAYOUT_UNDEFINED,
                              VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    auto depth_attachment = vk::util::DepthRenderingAttachmentInfo(
        depth_img.view, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    auto render_info =
        vk::util::RenderingInfo(gfx.GetSwapchainExtent(), &color_attachment, &depth_attachment);
    vkCmdBeginRendering(cmd, &render_info);

    auto view_proj = camera.GetViewProj();

    glm::vec3 box_size = simulation.GetBoundingBox().size;
    box_transform.SetScale(box_size);
    box_transform.SetPosition(-box_size / 2.0f);
    sim_transform.SetPosition(-box_size / 2.0f);

    auto pc = BoxDrawPipeline::PushConstants{
        .color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f),
        .matrix = view_proj * transform.Matrix() * box_transform.Matrix(),
    };
    box_pipeline.Draw(cmd, gfx, draw_img, pc);

    const auto& buffers = simulation.GetFrameData(current_frame);
    auto pos_buffer = buffers.position_buffer.device_addr;
    auto vel_buffer = buffers.velocity_buffer.device_addr;

    auto pc_particles = Particle3DDrawPipeline::PushConstants{
        .matrix = view_proj * transform.Matrix() * sim_transform.Matrix(),
        .proj = camera.GetProj(),
        .positions = pos_buffer,
        .velocities = vel_buffer,
    };

    particles_pipeline.Draw(cmd, gfx, draw_img, particle_mesh, pc_particles,
                            simulation.GetParameters().n_particles);

    vkCmdEndRendering(cmd);
}

void SimulationRenderer3D::Clear(const gfx::Device& gfx) {
    gfx::DestroyMesh(gfx, particle_mesh);
    gfx.DestroyImage(draw_img);
    gfx.DestroyImage(depth_img);
    particles_pipeline.Clear(gfx.GetCoreCtx());
    box_pipeline.Clear(gfx.GetCoreCtx());
}

Transform::Transform(const glm::mat4& matrix) {
    glm::vec3 out_scale;
    glm::quat out_rotation;
    glm::vec3 out_translation;
    glm::vec3 out_skew;
    glm::vec4 out_perspective;
    glm::decompose(matrix, out_scale, out_rotation, out_translation, out_skew, out_perspective);

    position = out_translation;
    rotation = out_rotation;
    scale = out_scale;
    dirty = true;
}

void Transform::SetPosition(const glm::vec3& p) {
    position = p;
    dirty = true;
}

void Transform::SetRotation(const glm::quat& q) {
    rotation = glm::normalize(q);
    dirty = true;
}

void Transform::SetScale(const glm::vec3& s) {
    scale = s;
    dirty = true;
}

const glm::mat4& Transform::Matrix() const {
    if (!dirty) {
        return transform;
    }

    transform = glm::translate(glm::mat4{1.0f}, position);
    if (rotation != glm::identity<glm::quat>()) {
        transform *= glm::mat4_cast(rotation);
    }
    transform = glm::scale(transform, scale);
    dirty = false;
    return transform;
}

bool Transform::IsIdentity() const {
    return Matrix() == glm::mat4{1.0f};
}

Transform Transform::Inverse() const {
    if (IsIdentity()) {
        return Transform{};
    }

    return Transform(glm::inverse(Matrix()));
}

void Camera::SetPerspective(float fov_x, float z_near, float z_far, float ar) {
    proj_type = ProjType::Perspective;

    fov.x = fov_x;
    aspect_ratio = ar;

    float g = aspect_ratio / glm::tan(fov.x / 2.0);
    fov.y = 2.0f * glm::atan(1.0f / g);

    if (inverse_depth) {
        projection = glm::perspective(fov.y, aspect_ratio, z_far, z_near);
    } else {
        projection = glm::perspective(fov.y, aspect_ratio, z_near, z_far);
    }

    if (clip_space_y_down) {
        projection[1][1] *= -1;
    }
}

// If this is set to true, the .clearValue.depthStencil.depth value in the should be
// set to 0.0f. Also, the compare op should be set to VK_COMPARE_OP_GREATER_OR_EQUAL in the pipeline
// creation
void Camera::SetInverseDepth(bool inverse) {
    inverse_depth = inverse;
    Reset();
}

void Camera::SetFoVX(float fovx) {
    this->fov.x = fovx;
    Reset();
}

void Camera::SetOrtho(float scale, float z_near, float z_far) {
    SetOrtho(glm::vec2(scale), z_near, z_far);
}

void Camera::SetOrtho(const glm::vec2& scale, float z_near, float z_far) {
    proj_type = ProjType::Orthographic;
    ortho_scale = scale;
    this->z_near = z_near;
    this->z_far = z_far;
    aspect_ratio = ortho_scale.x / ortho_scale.y;

    if (inverse_depth) {
        projection =
            glm::ortho(-ortho_scale.x, ortho_scale.x, -ortho_scale.y, ortho_scale.y, z_far, z_near);
    } else {
        projection =
            glm::ortho(-ortho_scale.x, ortho_scale.x, -ortho_scale.y, ortho_scale.y, z_near, z_far);
    }
}

void Camera::SetOrtho2D(const glm::vec2& size, float z_near, float z_far, OriginType origin) {
    proj_type = ProjType::Orthographic2D;
    clip_space_y_down = true;
    ortho_view_size = size;
    this->z_near = z_near;
    this->z_far = z_far;
    aspect_ratio = size.x / size.y;

    switch (origin) {
        case vfs::OriginType::TopLeft: {
            projection = glm::ortho(0.0f, size.x, 0.0f, size.y, z_near, z_far);
            break;
        }
        case vfs::OriginType::BottomLeft: {
            projection = glm::ortho(0.0f, size.x, size.y, 0.0f, z_near, z_far);
            break;
        }
        case vfs::OriginType::Center: {
            auto half = size / 2.0f;
            projection = glm::ortho(-half.x, half.x, half.y, -half.y, z_near, z_far);
            break;
        }
    }
}

void Camera::Reset() {
    switch (proj_type) {
        case vfs::ProjType::Orthographic: {
            SetOrtho(ortho_scale, z_near, z_far);
            break;
        }

        case vfs::ProjType::Orthographic2D: {
            SetOrtho2D(ortho_view_size, z_near, z_far);
            break;
        }

        case vfs::ProjType::Perspective: {
            SetPerspective(fov.x, z_near, z_far, aspect_ratio);
            break;
        }

        default:
            break;
    }
}

void Camera::SetPosition(const glm::vec3& pos) {
    transform.SetPosition(pos);
}

void Camera::SetPosition2D(const glm::vec2& pos) {
    transform.SetPosition(glm::vec3(pos, 0.0f));
}

glm::vec3 Camera::GetPosition() const {
    return transform.Position();
}

void Camera::SetRotation(const glm::quat& q) {
    transform.SetRotation(q);
}

void Camera::SetTarget(const glm::vec3& target) {
    transform.SetRotation(
        glm::quatLookAt(glm::normalize(transform.Position() - target), gfx::axis::UP));
}

const glm::quat& Camera::GetRotation() const {
    return transform.Rotation();
}

glm::mat4 Camera::GetView() const {
    if (proj_type == ProjType::Orthographic2D) {
        return glm::translate(glm::mat4{1.0f}, -transform.Position());
    }

    const auto up = transform.LocalUp();
    const auto pos = GetPosition();
    const auto target = pos + transform.LocalFront();
    return glm::lookAt(pos, target, up);
}

glm::mat4 Camera::GetViewProj() const {
    return projection * GetView();
}

const glm::mat4& Camera::GetProj() const {
    return projection;
}

}  // namespace vfs
