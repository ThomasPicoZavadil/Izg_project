/*!
 * @file
 * @brief This file contains implementation of gpu
 *
 * @author Tomáš Milet, imilet@fit.vutbr.cz
 */

#include <student/gpu.hpp>

void clear(GPUMemory &mem, ClearCommand cmd)
{
  // Výběr framebufferu
  Framebuffer *fbo = mem.framebuffers + mem.activatedFramebuffer;

  // Máme čistit barvu framebufferu?
  if (cmd.clearColor)
  {
    // Obsahuje framebuffer barevný buffer?
    if (fbo->color.data)
    {
      // Iterace přes všechny pixely
      for (int y = 0; y < fbo->height; ++y)
      {
        for (int x = 0; x < fbo->width; ++x)
        {
          // Pixel [x,y] začíná na adrese:
          uint8_t *pixelStart = ((uint8_t *)fbo->color.data) + y * fbo->color.pitch + x * fbo->color.bytesPerPixel;

          // Pokud jsou data typu float
          if (fbo->color.format == Image::FLOAT32)
          {
            float *pixelf = (float *)pixelStart;

            // Nastavení hodnot barevných kanálů
            pixelf[0] = cmd.color.r;
            pixelf[1] = cmd.color.g;
            pixelf[2] = cmd.color.b;
            pixelf[3] = cmd.color.a;
          }
          // Pokud jsou data typu uint8_t
          else if (fbo->color.format == Image::UINT8)
          {
            uint8_t *pixelu = (uint8_t *)pixelStart;

            // Nastavení hodnot barevných kanálů
            pixelu[0] = static_cast<uint8_t>(cmd.color.r * 255);
            pixelu[1] = static_cast<uint8_t>(cmd.color.g * 255);
            pixelu[2] = static_cast<uint8_t>(cmd.color.b * 255);
            pixelu[3] = static_cast<uint8_t>(cmd.color.a * 255);
          }
        }
      }
    }
  }

  // Máme čistit hloubku framebufferu?
  if (cmd.clearDepth)
  {
    // Obsahuje framebuffer depth buffer?
    if (fbo->depth.data)
    {
      // Iterace přes všechny pixely
      for (int y = 0; y < fbo->height; ++y)
      {
        for (int x = 0; x < fbo->width; ++x)
        {
          // Pixel [x,y] začíná na adrese:
          float *pixelStart = reinterpret_cast<float *>(static_cast<uint8_t *>(fbo->depth.data) + y * fbo->depth.pitch + x * fbo->depth.bytesPerPixel);

          // Nastavení hodnoty hloubky
          *pixelStart = cmd.depth;
        }
      }
    }
  }
}

void bindFramebuffer(GPUMemory &mem, BindFramebufferCommand cmd)
{
  mem.activatedFramebuffer = cmd.id;
}

void bindProgram(GPUMemory &mem, BindProgramCommand cmd)
{
  mem.activatedProgram = cmd.id;
}

void bindVertexArray(GPUMemory &mem, BindVertexArrayCommand cmd)
{
  mem.activatedVertexArray = cmd.id;
}

void draw(GPUMemory &mem, DrawCommand cmd)
{
  VertexShader vs = mem.programs[mem.activatedProgram].vertexShader;
  for (uint32_t i = 0; i < cmd.nofVertices; i++)
  {
    InVertex inVertex;
    OutVertex outVertex;
    ShaderInterface si;
    si.gl_DrawID = mem.gl_DrawID;
    vs(outVertex, inVertex, si);
  }
}

void setDrawId(GPUMemory &mem, SetDrawIdCommand cmd)
{
  mem.gl_DrawID = cmd.id;
}

void sub(GPUMemory &mem, SubCommand cmd)
{
}

//! [izg_enqueue]
void izg_enqueue(GPUMemory &mem, CommandBuffer const &cb)
{
  (void)mem;
  (void)cb;
  /// \todo Tato funkce reprezentuje funkcionalitu grafické karty.<br>
  /// Měla by umět zpracovat command buffer, čistit framebuffer a kresli.<br>
  /// mem obsahuje paměť grafické karty.
  /// cb obsahuje command buffer pro zpracování.
  /// Bližší informace jsou uvedeny na hlavní stránce dokumentace.
  mem.gl_DrawID = 0;
  for (uint32_t i = 0; i < cb.nofCommands; ++i)
  {
    CommandType type = cb.commands[i].type;
    CommandData data = cb.commands[i].data;
    if (type == CommandType::CLEAR)
      clear(mem, data.clearCommand);
    if (type == CommandType::BIND_FRAMEBUFFER)
      bindFramebuffer(mem, data.bindFramebufferCommand);
    if (type == CommandType::BIND_PROGRAM)
      bindProgram(mem, data.bindProgramCommand);
    if (type == CommandType::BIND_VERTEXARRAY)
      bindVertexArray(mem, data.bindVertexArrayCommand);
    if (type == CommandType::DRAW)
    {
      draw(mem, data.drawCommand);
      mem.gl_DrawID++;
    }
    if (type == CommandType::SET_DRAW_ID)
      setDrawId(mem, data.setDrawIdCommand);
    if (type == CommandType::SUB_COMMAND)
      sub(mem, data.subCommand);
  }
}
//! [izg_enqueue]

/**
 * @brief This function reads color from texture.
 *
 * @param texture texture
 * @param uv uv coordinates
 *
 * @return color 4 floats
 */
glm::vec4 read_texture(Texture const &texture, glm::vec2 uv)
{
  if (!texture.img.data)
    return glm::vec4(0.f);
  auto &img = texture.img;
  auto uv1 = glm::fract(glm::fract(uv) + 1.f);
  auto uv2 = uv1 * glm::vec2(texture.width - 1, texture.height - 1) + 0.5f;
  auto pix = glm::uvec2(uv2);
  return texelFetch(texture, pix);
}

/**
 * @brief This function reads color from texture with clamping on the borders.
 *
 * @param texture texture
 * @param uv uv coordinates
 *
 * @return color 4 floats
 */
glm::vec4 read_textureClamp(Texture const &texture, glm::vec2 uv)
{
  if (!texture.img.data)
    return glm::vec4(0.f);
  auto &img = texture.img;
  auto uv1 = glm::clamp(uv, 0.f, 1.f);
  auto uv2 = uv1 * glm::vec2(texture.width - 1, texture.height - 1) + 0.5f;
  auto pix = glm::uvec2(uv2);
  return texelFetch(texture, pix);
}

/**
 * @brief This function fetches color from texture.
 *
 * @param texture texture
 * @param pix integer coorinates
 *
 * @return color 4 floats
 */
glm::vec4 texelFetch(Texture const &texture, glm::uvec2 pix)
{
  auto &img = texture.img;
  glm::vec4 color = glm::vec4(0.f, 0.f, 0.f, 1.f);
  if (pix.x >= texture.width || pix.y >= texture.height)
    return color;
  if (img.format == Image::UINT8)
  {
    auto colorPtr = (uint8_t *)getPixel(img, pix.x, pix.y);
    for (uint32_t c = 0; c < img.channels; ++c)
      color[c] = colorPtr[img.channelTypes[c]] / 255.f;
  }
  if (texture.img.format == Image::FLOAT32)
  {
    auto colorPtr = (float *)getPixel(img, pix.x, pix.y);
    for (uint32_t c = 0; c < img.channels; ++c)
      color[c] = colorPtr[img.channelTypes[c]];
  }
  return color;
}
