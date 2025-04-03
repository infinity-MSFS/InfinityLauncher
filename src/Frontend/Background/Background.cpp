
#include "Background.hpp"

#include <iostream>
#include <ranges>
#include <unordered_map>

namespace Infinity {
  ImVec4 Background::m_primaryColor = HomePagePrimary;
  ImVec4 Background::m_secondaryColor = HomePageSecondary;
  ImVec4 Background::m_circleColor1 = {18.0f / 255.0f, 113.0f / 255.f, 1.0f, 0.002f};
  ImVec4 Background::m_circleColor2 = {221.0f / 255.f, 74.0f / 255.f, 1.0f, 0.002f};
  ImVec4 Background::m_circleColor3 = {100.0f / 255.f, 220.0f / 255.f, 1.0f, 0.002f};
  ImVec4 Background::m_circleColor4 = {200.0f / 255.f, 50.0f / 255.f, 50.0f / 255.f, 0.002f};
  ImVec4 Background::m_circleColor5 = {180.0f / 255.f, 180.0f / 255.f, 50.0f / 255.f, 0.002f};
  ImVec2 Background::m_windowPos = {0.0f, 0.0f};
  ImVec2 Background::m_windowSize = {0.0f, 0.0f};
  bool Background::m_HomePage = true;
  float Background::m_dotOpacity = 0.3f;
  float Background::m_targetDotOpacity = 0.3f;

  GLuint Background::m_dotVAO = 0;
  GLuint Background::m_dotVBO = 0;
  GLuint Background::m_dotShader = 0;
  int Background::m_dotCount = 0;
  bool Background::m_dotsInitialized = false;

  const std::unordered_map<std::string, ImVec2> defaultCirclePositions = {{"Circle1", ImVec2(100, 100)},
                                                                          {"Circle2", ImVec2(200, 200)},
                                                                          {"Circle3", ImVec2(300, 300)},
                                                                          {"Circle4", ImVec2(400, 400)},
                                                                          {"Circle5", ImVec2(200, 50)}};

  Background::~Background() { CleanupDots(); }

  Background::Background() {
    if (m_circlePos.empty()) {
      m_circlePos.resize(5);
    }
  }

  void Background::RenderBackground() {
    m_windowPos = ImGui::GetWindowPos();
    m_windowSize = ImGui::GetWindowSize();

    UpdateDotOpacity();
    RenderBackgroundBaseLayer();
    RenderBackgroundGradientLayer();
    RenderBackgroundDotsLayer();
  }


  void Background::RenderBackgroundBaseLayer() {
    ImGui::GetWindowDrawList()->AddRectFilled(m_windowPos,
                                              ImVec2(m_windowPos.x + m_windowSize.x, m_windowPos.y + m_windowSize.y),
                                              ImColor(0.0f, 0.0f, 0.0f, 1.0f));

    ImGui::GetWindowDrawList()->AddRectFilledMultiColor(
        m_windowPos, ImVec2(m_windowPos.x + m_windowSize.x, m_windowPos.y + m_windowSize.y), ImColor(m_primaryColor),
        ImColor(m_primaryColor), ImColor(m_secondaryColor), ImColor(m_secondaryColor));
  }

  void Background::RenderGradientCircle(const ImVec2 center, const float radius, const float maxOpacity,
                                        const ImU32 color) {
    constexpr int layers = 80;

    const ImVec4 colorVec = ImGui::ColorConvertU32ToFloat4(color);

    for (int i = 0; i < layers; i++) {
      constexpr int segments = 150;

      const float layerRadius = radius * (layers - static_cast<float>(i)) / layers;
      float alpha = colorVec.w * (static_cast<float>(i) + 1.0f) / layers;

      if (alpha > maxOpacity) {
        alpha = maxOpacity;
      }

      const ImU32 layerColor = ImGui::ColorConvertFloat4ToU32(ImVec4(colorVec.x, colorVec.y, colorVec.z, alpha));

      ImGui::GetWindowDrawList()->AddCircleFilled(center, layerRadius, layerColor, segments);
    }
  }

  void Background::TrySetDefaultPositions() {
    if (m_circlePos.size() == 5) {
      int i = 0;
      for (const auto& val: defaultCirclePositions | std::views::values) {
        InitializeCirclePosition(i++, val);
        if (i >= 5) break;
      }
    }
  }

  void Background::SetDotOpacity(float opacity) { m_targetDotOpacity = opacity; }

  void Background::UpdateDotOpacity() {
    constexpr float transitionSpeed = 0.003f;

    if (m_dotOpacity < m_targetDotOpacity) {
      m_dotOpacity += transitionSpeed;
      if (m_dotOpacity > m_targetDotOpacity) {
        m_dotOpacity = m_targetDotOpacity;
      }
    } else if (m_dotOpacity > m_targetDotOpacity) {
      m_dotOpacity -= transitionSpeed;
      if (m_dotOpacity < m_targetDotOpacity) {
        m_dotOpacity = m_targetDotOpacity;
      }
    }
  }


  void Background::RenderBackgroundDotsLayer() {
    if (!m_dotsInitialized) {
      InitializeDots();
    }

    if (!glIsBuffer(m_dotVBO)) {
      std::cerr << "Error: m_dotVBO is not a valid buffer!" << std::endl;
    }
    if (!glIsVertexArray(m_dotVAO)) {
      std::cerr << "Error: m_dotVAO is not a valid vertex array!" << std::endl;
    }

    glUseProgram(m_dotShader);

    GLint projectionLoc = glGetUniformLocation(m_dotShader, "projection");
    GLint dotRadiusLoc = glGetUniformLocation(m_dotShader, "dotRadius");
    GLint dotColorLoc = glGetUniformLocation(m_dotShader, "dotColor");

    if (projectionLoc == -1) std::cerr << "Uniform 'projection' not found!" << std::endl;
    if (dotRadiusLoc == -1) std::cerr << "Uniform 'dotRadius' not found!" << std::endl;
    if (dotColorLoc == -1) std::cerr << "Uniform 'dotColor' not found!" << std::endl;

    float projection[16] = {2.0f / m_windowSize.x,
                            0.0f,
                            0.0f,
                            0.0f,
                            0.0f,
                            2.0f / -m_windowSize.y,
                            0.0f,
                            0.0f,
                            0.0f,
                            0.0f,
                            -1.0f,
                            0.0f,
                            -1.0f,
                            1.0f,
                            0.0f,
                            1.0f};

    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, projection);
    glUniform1f(dotRadiusLoc, 1.0f);
    glUniform4f(dotColorLoc, 1.0f, 1.0f, 1.0f, m_dotOpacity);

    glBindBuffer(GL_ARRAY_BUFFER, m_dotVBO);
    std::vector<float> updated_offsets(m_dotCount * 2);
    const auto offset_pos = ImVec2(m_windowPos.x + 10, m_windowPos.y + 10);
    const auto dot_count_2D = ImVec2(300, 150);
    constexpr float spacing = 15.0f;

    int index = 0;
    for (int y = 0; y < static_cast<int>(dot_count_2D.y); y++) {
      for (int x = 0; x < static_cast<int>(dot_count_2D.x); x++) {
        updated_offsets[index++] = offset_pos.x + static_cast<float>(x) * spacing;
        updated_offsets[index++] = offset_pos.y + static_cast<float>(y) * spacing;
      }
    }

    glBufferSubData(GL_ARRAY_BUFFER, 0, updated_offsets.size() * sizeof(float), updated_offsets.data());

    glBindVertexArray(m_dotVAO);
    glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 10, m_dotCount);
    glBindVertexArray(0);
    ImGui::GetIO().BackendRendererUserData;
  }

  void Background::RenderBackgroundGradientLayer() {
    static float circle1angle = 0.0f;
    static float circle2angle = 0.0f;
    static float circle3angle = 0.0f;
    static float circle4angle = 0.0f;
    static float circle5angle = 0.0f;

    TrySetDefaultPositions();


    RenderGradientCircle(ImVec2(m_windowPos.x + m_circlePos[0].x, m_windowPos.y + m_circlePos[0].y), 600.0f, 0.01f,
                         ImColor(m_circleColor1));
    RenderGradientCircle(ImVec2(m_windowPos.x + m_circlePos[1].x, m_windowPos.y + m_circlePos[1].y), 400.0f, 0.01f,
                         ImColor(m_circleColor2));
    RenderGradientCircle(ImVec2(m_windowPos.x + m_circlePos[2].x, m_windowPos.y + m_circlePos[2].y), 600.0f, 0.01f,
                         ImColor(m_circleColor3));
    RenderGradientCircle(ImVec2(m_windowPos.x + m_circlePos[3].x, m_windowPos.y + m_circlePos[3].y), 400.0f, 0.01f,
                         ImColor(m_circleColor4));
    RenderGradientCircle(ImVec2(m_windowPos.x + m_circlePos[4].x, m_windowPos.y + m_circlePos[4].y), 600.0f, 0.01f,
                         ImColor(m_circleColor5));


    if (circle1angle - 0.05f < 0.0f)
      circle1angle = 360.0f;
    else
      circle1angle -= 0.05f;
    if (circle2angle + 0.05f > 360)
      circle2angle = 0;
    else
      circle2angle += 0.05f;
    if (circle3angle - 0.05f < 0.0f)
      circle3angle = 360.0f;
    else
      circle3angle -= 0.05f;
    if (circle4angle + 0.05f > 360)
      circle4angle = 0;
    else
      circle4angle += 0.05f;
    if (circle5angle - 0.05f < 360)
      circle5angle = 0;
    else
      circle5angle -= 0.05f;

    const auto screen_center = ImVec2(ImGui::GetWindowSize().x / 2.0f, ImGui::GetWindowSize().y / 2.0f);

    m_circlePos[0] = GetCircleCoords(600.0f, circle1angle, ImVec2(screen_center.x - 100.0f, screen_center.y + 200.0f));
    m_circlePos[1] = GetCircleCoords(500.0f, circle2angle, ImVec2(screen_center.x + 100.0f, screen_center.y + 20.0f));
    m_circlePos[2] = GetCircleCoords(155.0f, circle3angle, ImVec2(screen_center.x - 200.0f, screen_center.y - 100.0f));
    m_circlePos[3] = GetCircleCoords(409.0f, circle4angle, ImVec2(screen_center.x + 0.0f, screen_center.y + 200.0f));
    m_circlePos[4] = GetCircleCoords(781.0f, circle5angle, ImVec2(screen_center.x - 100.0f, screen_center.y + 0.0f));
  }

  void Background::InitializeDots() {
    if (m_dotsInitialized) {
      return;
    }
    const char* vertexShaderSouce = R"(
#version 330 core
    layout (location = 0) in vec2 aPos;
    layout (location = 1) in vec2 aOffset;

    uniform mat4 projection;
    uniform float dotRadius;

    void main(){
      vec2 worldPos = aPos * dotRadius + aOffset;
      gl_Position = projection * vec4(worldPos, 0.0, 1.0);
    }
)";

    const char* fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;

        uniform vec4 dotColor;

        void main() {
            // Calculate distance from center (0,0) of this fragment
            vec2 circCoord = gl_PointCoord * 2.0 - 1.0;
            float distSq = dot(circCoord, circCoord);

            // Discard fragments outside circle
            if (distSq > 1.0) discard;

            FragColor = dotColor;
        }
    )";

    m_dotShader = CreateShader(vertexShaderSouce, fragmentShaderSource);

    const int segments = 8;
    std::vector<float> vertices;
    vertices.reserve((segments + 2) * 2);

    vertices.push_back(0.0f);
    vertices.push_back(0.0f);

    for (int i = 0; i <= segments; i++) {
      float angle = 2.0f * M_PI * static_cast<float>(i) / static_cast<float>(segments);
      vertices.push_back(cos(angle));
      vertices.push_back(sin(angle));
    }

    const auto dotCount2D = ImVec2(300, 150);
    std::vector<float> instanceData;
    instanceData.reserve(static_cast<size_t>(dotCount2D.x) * static_cast<size_t>(dotCount2D.y) * 2);

    constexpr float spacing = 15.0f;
    for (int y = 0; y < static_cast<int>(dotCount2D.y); y++) {
      for (int x = 0; x < static_cast<int>(dotCount2D.x); x++) {
        instanceData.push_back(static_cast<float>(x) * spacing);
        instanceData.push_back(static_cast<float>(y) * spacing);
      }
    }

    m_dotCount = static_cast<int>(instanceData.size() / 2);

    glGenVertexArrays(1, &m_dotVAO);
    glBindVertexArray(m_dotVAO);

    GLuint vertexVBO;
    glGenBuffers(1, &vertexVBO);
    glBindBuffer(GL_ARRAY_BUFFER, vertexVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*) 0);

    glGenBuffers(1, &m_dotVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_dotVBO);
    glBufferData(GL_ARRAY_BUFFER, instanceData.size() * sizeof(float), instanceData.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*) 0);
    glVertexAttribDivisor(1, 1);

    glBindVertexArray(0);
    m_dotsInitialized = true;
  }

  void Background::CleanupDots() {
    if (m_dotsInitialized) {
      glDeleteVertexArrays(1, &m_dotVAO);
      glDeleteBuffers(1, &m_dotVBO);
      glDeleteProgram(m_dotShader);
      m_dotsInitialized = false;
    }
  }


  GLuint Background::CreateShader(const char* vertex_shader_source, const char* fragment_shader_source) {
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_source, nullptr);
    glCompileShader(vertex_shader);

    GLint success;
    GLchar info_log[512];
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
      glGetShaderInfoLog(vertex_shader, 512, nullptr, info_log);
      std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << info_log << std::endl;
      return 0;
    }

    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_source, nullptr);
    glCompileShader(fragment_shader);

    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
      glGetShaderInfoLog(fragment_shader, 512, nullptr, info_log);
      std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << info_log << std::endl;
      glDeleteShader(vertex_shader);
      return 0;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertex_shader);
    glAttachShader(shaderProgram, fragment_shader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
      glGetProgramInfoLog(shaderProgram, 512, nullptr, info_log);
      std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << info_log << std::endl;
      glDeleteShader(vertex_shader);
      glDeleteShader(fragment_shader);
      return 0;
    }
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    return shaderProgram;
  }


}  // namespace Infinity
