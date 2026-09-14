#include "platform/switch/SwitchGpuCapture.h"

#include "Cafe/HW/Latte/Core/Latte.h"
#include "Cafe/HW/Latte/Core/LatteCachedFBO.h"
#include "Cafe/HW/Latte/Core/LatteShader.h"
#include "Cafe/HW/Latte/Core/LatteTexture.h"
#include "Cafe/HW/Latte/LegacyShaderDecompiler/LatteDecompiler.h"
#include "Cafe/CafeSystem.h"
#include "Cemu/Logging/CemuLogging.h"
#include "config/ActiveSettings.h"
#include "util/helpers/StringBuf.h"

#include <atomic>
#include <ctime>
#include <fstream>
#include <set>
#include <system_error>

namespace
{
	constexpr uint32_t kMaxTracedDraws = 4096;
	constexpr size_t kShaderByteBudget = 24u * 1024 * 1024;

	std::atomic_bool s_requested{false};
	bool s_capturing = false;
	uint32_t s_drawIndex = 0;
	uint32_t s_tracedDraws = 0;
	size_t s_shaderBytes = 0;
	std::ofstream s_trace;
	fs::path s_directory;
	std::set<uint64_t> s_writtenShaders;

	const char* PrimitiveName(uint32_t primitiveType)
	{
		switch (static_cast<Latte::LATTE_VGT_PRIMITIVE_TYPE::E_PRIMITIVE_TYPE>(primitiveType))
		{
		case Latte::LATTE_VGT_PRIMITIVE_TYPE::E_PRIMITIVE_TYPE::POINTS: return "points";
		case Latte::LATTE_VGT_PRIMITIVE_TYPE::E_PRIMITIVE_TYPE::LINES: return "lines";
		case Latte::LATTE_VGT_PRIMITIVE_TYPE::E_PRIMITIVE_TYPE::LINE_STRIP: return "line_strip";
		case Latte::LATTE_VGT_PRIMITIVE_TYPE::E_PRIMITIVE_TYPE::TRIANGLES: return "triangles";
		case Latte::LATTE_VGT_PRIMITIVE_TYPE::E_PRIMITIVE_TYPE::TRIANGLE_FAN: return "triangle_fan";
		case Latte::LATTE_VGT_PRIMITIVE_TYPE::E_PRIMITIVE_TYPE::TRIANGLE_STRIP: return "triangle_strip";
		case Latte::LATTE_VGT_PRIMITIVE_TYPE::E_PRIMITIVE_TYPE::QUADS: return "quads";
		case Latte::LATTE_VGT_PRIMITIVE_TYPE::E_PRIMITIVE_TYPE::QUAD_STRIP: return "quad_strip";
		case Latte::LATTE_VGT_PRIMITIVE_TYPE::E_PRIMITIVE_TYPE::RECTS: return "rects";
		default: return "other";
		}
	}

	void WriteShaderSource(LatteDecompilerShader* shader, const char* stage)
	{
		if (!shader || !shader->strBuf_shaderSource)
			return;
		const uint64_t key = shader->baseHash ^ (shader->auxHash * 0x9E3779B97F4A7C15ull);
		if (!s_writtenShaders.insert(key).second)
			return;
		const size_t length = shader->strBuf_shaderSource->getLen();
		if (s_shaderBytes + length > kShaderByteBudget)
		{
			s_trace << "# shader budget exhausted, " << stage << ' '
					<< std::hex << shader->baseHash << '_' << shader->auxHash << std::dec << " not written\n";
			return;
		}
		std::ofstream out(s_directory / fmt::format("{:016x}_{:016x}_{}.txt", shader->baseHash, shader->auxHash, stage),
						  std::ios::binary);
		if (!out)
			return;
		out.write(shader->strBuf_shaderSource->c_str(), static_cast<std::streamsize>(length));
		s_shaderBytes += length;
	}

	void TraceShader(LatteDecompilerShader* shader, const char* stage)
	{
		if (!shader)
		{
			s_trace << "  " << stage << "=none\n";
			return;
		}
		WriteShaderSource(shader, stage);
		s_trace << "  " << stage << '=' << std::hex << shader->baseHash << '_' << shader->auxHash << std::dec
				<< " textures=" << (int)shader->textureUnitListCount << '\n';
	}

	void TraceTexture(const char* label, LatteTextureView* view)
	{
		if (!view || !view->baseTexture)
			return;
		const LatteTexture* texture = view->baseTexture;
		s_trace << "  " << label << " addr=0x" << std::hex << texture->physAddress
				<< " fmt=0x" << (uint32_t)texture->format << std::dec
				<< ' ' << texture->width << 'x' << texture->height
				<< (texture->isDepth ? " depth" : "") << '\n';
	}
}

void SwitchGpuCapture_Request()
{
	s_requested.store(true, std::memory_order_release);
}

bool SwitchGpuCapture_IsCapturing()
{
	return s_capturing;
}

void SwitchGpuCapture_BeginFrame()
{
	if (s_capturing || !s_requested.exchange(false, std::memory_order_acq_rel))
		return;

	s_directory = ActiveSettings::GetUserDataPath("dump/captures/{:016x}_{}",
												  CafeSystem::GetForegroundTitleId(), (uint64_t)std::time(nullptr));
	std::error_code error;
	fs::create_directories(s_directory, error);
	if (error)
	{
		cemuLog_log(LogType::Force, "GPU capture: could not create {}", _pathToUtf8(s_directory));
		return;
	}
	s_trace.open(s_directory / "trace.txt", std::ios::binary);
	if (!s_trace)
	{
		cemuLog_log(LogType::Force, "GPU capture: could not open the trace file");
		return;
	}

	s_capturing = true;
	s_drawIndex = 0;
	s_tracedDraws = 0;
	s_shaderBytes = 0;
	s_writtenShaders.clear();

	s_trace << "# Cemu-nx GPU capture\n"
			<< "title=" << std::hex << CafeSystem::GetForegroundTitleId() << std::dec << '\n'
			<< "renderer=" << (ActiveSettings::GetGraphicsAPI() == kVulkan ? "vulkan" : "opengl") << '\n';
	cemuLog_log(LogType::Force, "GPU capture: recording a frame to {}", _pathToUtf8(s_directory));
}

void SwitchGpuCapture_NoteDraw(uint32_t count, uint32_t instanceCount, uint32_t baseVertex, bool isIndexed)
{
	if (!s_capturing)
		return;
	const uint32_t index = s_drawIndex++;
	if (s_tracedDraws >= kMaxTracedDraws)
		return;
	s_tracedDraws++;

	const auto& reg = LatteGPUState.contextNew;
	s_trace << "\n[draw " << index << "] " << PrimitiveName(LatteGPUState.contextRegister[mmVGT_PRIMITIVE_TYPE])
			<< " count=" << count << " instances=" << instanceCount
			<< " baseVertex=" << baseVertex << (isIndexed ? " indexed\n" : " sequential\n");

	TraceShader(LatteSHRC_GetActiveVertexShader(), "vs");
	TraceShader(LatteSHRC_GetActiveGeometryShader(), "gs");
	TraceShader(LatteSHRC_GetActivePixelShader(), "ps");

	for (uint32_t i = 0; i < 8; i++)
		TraceTexture(fmt::format("rt{}", i).c_str(), LatteMRT::GetColorAttachment(i));
	TraceTexture("depth", LatteMRT::GetDepthAttachment());

	const auto depthControl = reg.DB_DEPTH_CONTROL;
	s_trace << "  state depth_test=" << depthControl.get_Z_ENABLE()
			<< " depth_write=" << depthControl.get_Z_WRITE_ENABLE()
			<< " stencil=" << depthControl.get_STENCIL_ENABLE()
			<< " cull_front=" << reg.PA_SU_SC_MODE_CNTL.get_CULL_FRONT()
			<< " cull_back=" << reg.PA_SU_SC_MODE_CNTL.get_CULL_BACK() << '\n';
}

void SwitchGpuCapture_EndFrame()
{
	if (!s_capturing)
		return;
	s_trace << "\n# draws=" << s_drawIndex;
	if (s_drawIndex > s_tracedDraws)
		s_trace << " (traced " << s_tracedDraws << ", the rest exceeded the trace limit)";
	s_trace << "\n# shaders=" << s_writtenShaders.size() << " bytes=" << s_shaderBytes << '\n';
	s_trace.close();
	s_capturing = false;
	cemuLog_log(LogType::Force, "GPU capture: {} draws, {} shaders written to {}",
				s_drawIndex, s_writtenShaders.size(), _pathToUtf8(s_directory));
}
