#pragma once
static float border_bang = 1.0f;
struct TabInfo
{
	ImRect bb;
	unsigned int index;
};
std::vector<TabInfo> tabs_info;
static int old_tab_index = 0;
bool tab_info_already_exist(std::vector<TabInfo> infos, unsigned int index)
{
	for (int i = 0; i < infos.size(); i++)
		if (infos[i].index == index) return true;

	return false;
}
float clip(float n, float lower, float upper)
{
	n = (n > lower) * n + !(n > lower) * lower;
	return (n < upper) * n + !(n < upper) * upper;
}
namespace ImGui
{
	static inline float ObliqueSliderBehaviorCalcRatioFromValue(float v, float v_min, float v_max, float power, float linear_zero_pos)
	{
		if (v_min == v_max)
			return 0.0f;

		const bool is_non_linear = (power < 1.0f - 0.00001f) || (power > 1.0f + 0.00001f);
		const float v_clamped = (v_min < v_max) ? ImClamp(v, v_min, v_max) : ImClamp(v, v_max, v_min);
		if (is_non_linear)
		{
			if (v_clamped < 0.0f)
			{
				const float f = 1.0f - (v_clamped - v_min) / (ImMin(0.0f, v_max) - v_min);
				return (1.0f - powf(f, 1.0f / power)) * linear_zero_pos;
			}
			else
			{
				const float f = (v_clamped - ImMax(0.0f, v_min)) / (v_max - ImMax(0.0f, v_min));
				return linear_zero_pos + powf(f, 1.0f / power) * (1.0f - linear_zero_pos);
			}
		}

		// Linear slider
		return (v_clamped - v_min) / (v_max - v_min);
	}
	IMGUI_API bool ObliqueSliderBehavior(const ImRect& frame_bb, ImGuiID id, float* v, float v_min, float v_max, float power, int decimal_precision, ImGuiSliderFlags flags = 0)
	{
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = GetCurrentWindow();
		const ImGuiStyle& style = g.Style;

		// Draw frame
		//RenderFrame(frame_bb.Min, frame_bb.Max, GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);
		//
		ImRect bbMin(ImVec2(frame_bb.Min.x + 6, frame_bb.Min.y), ImVec2(frame_bb.Min.x, frame_bb.Max.y));
		ImRect bbMax(ImVec2(frame_bb.Max.x, frame_bb.Max.y), ImVec2(frame_bb.Max.x + 6, frame_bb.Min.y));
		window->DrawList->AddQuadFilled(bbMin.Min, bbMin.Max, bbMax.Min, bbMax.Max, GetColorU32(ImGuiCol_FrameBg));
		

		const bool is_non_linear = (power < 1.0f - 0.00001f) || (power > 1.0f + 0.00001f);
		const bool is_horizontal = (flags & ImGuiSliderFlags_Vertical) == 0;

		const float grab_padding = 0.0f;
		const float slider_sz = is_horizontal ? (frame_bb.GetWidth() - grab_padding * 2.0f) : (frame_bb.GetHeight() - grab_padding * 2.0f);
		float grab_sz;
		if (decimal_precision != 0)
			grab_sz = ImMin(style.GrabMinSize, slider_sz);
		else
			grab_sz = ImMin(ImMax(1.0f * (slider_sz / ((v_min < v_max ? v_max - v_min : v_min - v_max) + 1.0f)), style.GrabMinSize), slider_sz);  // Integer sliders, if possible have the grab size represent 1 unit
		const float slider_usable_sz = slider_sz - grab_sz;
		const float slider_usable_pos_min = (is_horizontal ? frame_bb.Min.x : frame_bb.Min.y) + grab_padding + grab_sz * 0.5f;
		const float slider_usable_pos_max = (is_horizontal ? frame_bb.Max.x : frame_bb.Max.y) - grab_padding - grab_sz * 0.5f;

		// For logarithmic sliders that cross over sign boundary we want the exponential increase to be symmetric around 0.0f
		float linear_zero_pos = 0.0f;   // 0.0->1.0f
		if (v_min * v_max < 0.0f)
		{
			// Different sign
			const float linear_dist_min_to_0 = powf(fabsf(0.0f - v_min), 1.0f / power);
			const float linear_dist_max_to_0 = powf(fabsf(v_max - 0.0f), 1.0f / power);
			linear_zero_pos = linear_dist_min_to_0 / (linear_dist_min_to_0 + linear_dist_max_to_0);
		}
		else
		{
			// Same sign
			linear_zero_pos = v_min < 0.0f ? 1.0f : 0.0f;
		}

		// Process clicking on the slider
		bool value_changed = false;
		if (g.ActiveId == id)
		{
			bool set_new_value = false;
			float clicked_t = 0.0f;
			if (g.IO.MouseDown[0])
			{
				const float mouse_abs_pos = is_horizontal ? g.IO.MousePos.x : g.IO.MousePos.y;
				clicked_t = (slider_usable_sz > 0.0f) ? ImClamp((mouse_abs_pos - slider_usable_pos_min) / slider_usable_sz, 0.0f, 1.0f) : 0.0f;
				if (!is_horizontal)
					clicked_t = 1.0f - clicked_t;
				set_new_value = true;
			}
			else
			{
				ClearActiveID();
			}

			if (set_new_value)
			{
				float new_value;
				if (is_non_linear)
				{
					// Account for logarithmic scale on both sides of the zero
					if (clicked_t < linear_zero_pos)
					{
						// Negative: rescale to the negative range before powering
						float a = 1.0f - (clicked_t / linear_zero_pos);
						a = powf(a, power);
						new_value = ImLerp(ImMin(v_max, 0.0f), v_min, a);
					}
					else
					{
						// Positive: rescale to the positive range before powering
						float a;
						if (fabsf(linear_zero_pos - 1.0f) > 1.e-6f)
							a = (clicked_t - linear_zero_pos) / (1.0f - linear_zero_pos);
						else
							a = clicked_t;
						a = powf(a, power);
						new_value = ImLerp(ImMax(v_min, 0.0f), v_max, a);
					}
				}
				else
				{
					// Linear slider
					new_value = ImLerp(v_min, v_max, clicked_t);
				}

				// Round past decimal precision
				new_value = RoundScalar(new_value, decimal_precision);
				if (*v != new_value)
				{
					*v = new_value;
					value_changed = true;
				}
			}
		}

		// Draw
		float grab_t = ObliqueSliderBehaviorCalcRatioFromValue(*v, v_min, v_max, power, linear_zero_pos);
		if (!is_horizontal)
			grab_t = 1.0f - grab_t;
		const float grab_pos = ImLerp(slider_usable_pos_min, slider_usable_pos_max, grab_t);
		ImRect grab_bb;
		if (is_horizontal)
			grab_bb = ImRect(ImVec2(grab_pos - grab_sz * 0.5f, frame_bb.Min.y + grab_padding), ImVec2(grab_pos + grab_sz * 0.5f, frame_bb.Max.y - grab_padding));
		else
			grab_bb = ImRect(ImVec2(frame_bb.Min.x + grab_padding, grab_pos - grab_sz * 0.5f), ImVec2(frame_bb.Max.x - grab_padding, grab_pos + grab_sz * 0.5f));


		ImRect grabMin(ImVec2(grab_bb.Min.x + 6, frame_bb.Min.y), ImVec2(grab_bb.Min.x, frame_bb.Max.y));
		ImRect grabMax(ImVec2(grab_bb.Max.x, frame_bb.Max.y), ImVec2(grab_bb.Max.x + 6, frame_bb.Min.y));
		window->DrawList->AddQuadFilled(bbMin.Min, bbMin.Max, grabMax.Min, grabMax.Max, GetColorU32(g.ActiveId == id ? ImGuiCol_Border : ImGuiCol_Border));
		//window->DrawList->AddQuad(bbMin.Min, bbMin.Max, bbMax.Min, bbMax.Max, GetColorU32(ImVec4(1,1,1,1)));
		window->DrawList->AddQuad(bbMin.Min, bbMin.Max, bbMax.Min, bbMax.Max, GetColorU32(ImGuiCol_Border));


		//window->DrawList->AddRectFilled(grab_bb.Min, grab_bb.Max, GetColorU32(g.ActiveId == id ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab), style.GrabRounding);

		return value_changed;
	}

	// Use power!=1.0 for logarithmic sliders.
	// Adjust display_format to decorate the value with a prefix or a suffix.
	//   "%.3f"         1.234
	//   "%5.2f secs"   01.23 secs
	//   "Gold: %.0f"   Gold: 1
	IMGUI_API bool ObliqueSliderFloat(const char* label, float* v, float v_min, float v_max, const char* display_format = xorstr("%.3f"), float power = 1.0f)
	{
		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return false;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const ImGuiID id = window->GetID(label);
		const float w = CalcItemWidth();

		const ImVec2 label_size = CalcTextSize(label, NULL, true);
		const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y * 2.0f));
		const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));

		// NB- we don't call ItemSize() yet because we may turn into a text edit box below
		if (!ItemAdd(total_bb, id))
		{
			ItemSize(total_bb, style.FramePadding.y);
			return false;
		}
		const bool hovered = ItemHoverable(frame_bb, id);

		if (!display_format)
			display_format = xorstr("%.3f");
		int decimal_precision = ParseFormatPrecision(display_format, 3);

		// Tabbing or CTRL-clicking on Slider turns it into an input box
		bool start_text_input = false;
		const bool tab_focus_requested = FocusableItemRegister(window, id);
		if (tab_focus_requested || (hovered && g.IO.MouseClicked[0]))
		{
			SetActiveID(id, window);
			FocusWindow(window);
			if (tab_focus_requested || g.IO.KeyCtrl)
			{
				start_text_input = true;
				g.ScalarAsInputTextId = 0;
			}
		}
		if (start_text_input || (g.ActiveId == id && g.ScalarAsInputTextId == id))
			return InputScalarAsWidgetReplacement(frame_bb, label, ImGuiDataType_Float, v, id, decimal_precision);

		// Actual slider behavior + render grab
		ItemSize(total_bb, style.FramePadding.y);
		const bool value_changed = ObliqueSliderBehavior(frame_bb, id, v, v_min, v_max, power, decimal_precision);

		// Display value using user-provided display format so user can add prefix/suffix/decorations to the value.
		char value_buf[64];
		const char* value_buf_end = value_buf + ImFormatString(value_buf, IM_ARRAYSIZE(value_buf), display_format, *v);
		RenderTextClipped(frame_bb.Min, frame_bb.Max, value_buf, value_buf_end, NULL, ImVec2(0.5f, 0.5f));

		if (label_size.x > 0.0f)
			RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);

		return value_changed;
	}


	IMGUI_API bool TabButton(const char* label, const ImVec2& size_arg, unsigned int index, ImGuiButtonFlags flags = 0)
	{
		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return false;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const ImGuiID id = window->GetID(label);
		const ImVec2 label_size = CalcTextSize(label, NULL, true);

		ImVec2 pos = window->DC.CursorPos;
		if ((flags & ImGuiButtonFlags_AlignTextBaseLine) && style.FramePadding.y < window->DC.CurrentLineTextBaseOffset) // Try to vertically align buttons that are smaller/have no padding so that text baseline matches (bit hacky, since it shouldn't be a flag)
			pos.y += window->DC.CurrentLineTextBaseOffset - style.FramePadding.y;
		ImVec2 size = CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

		const ImRect bb(pos, pos + size);
		ItemSize(bb, style.FramePadding.y);
		if (!ItemAdd(bb, id))
			return false;

		if (window->DC.ItemFlags & ImGuiItemFlags_ButtonRepeat)
			flags |= ImGuiButtonFlags_Repeat;
		bool hovered, held;
		bool pressed = ButtonBehavior(bb, id, &hovered, &held, flags);

		// Render
		const ImU32 col = GetColorU32((hovered && held) ? ImGuiCol_TabButtonActive : hovered ? ImGuiCol_TabButtonHovered : ImGuiCol_TabButton);
		//RenderFrame(bb.Min, bb.Max, col, false, style.FrameRounding);
		const ImU32 border_col = GetColorU32((hovered && held) ? GetColorU32(ImVec4(1, 1, 1, 0.7)) : hovered ? GetColorU32(ImVec4(Global::RGB.x, Global::RGB.y, Global::RGB.z, 0.45)) : GetColorU32(ImVec4(1, 1, 1, 0.7)));
		
		window->DrawList->AddRectFilledMultiColor(bb.Min, bb.Max, col, col, GetColorU32(ImVec4(30/255, 30/255, 30/255, 1)), GetColorU32(ImVec4(30/255, 30/255, 30/255, 1)));
		window->DrawList->AddRect(bb.Min, bb.Max, border_col, style.FrameRounding, ImDrawCornerFlags_All, 1.0f);
		RenderTextClipped(bb.Min + style.FramePadding, bb.Max - style.FramePadding, label, NULL, &label_size, style.ButtonTextAlign, &bb);
		if (!tab_info_already_exist(tabs_info, index))
		{
			TabInfo tab_info;
			tab_info.bb = bb;
			tab_info.index = index;
			tabs_info.push_back(tab_info);
		}
		
		return pressed;
	}
	IMGUI_API bool Tab(unsigned int index, const char* label, int* selected, float width = 0)
	{
		/*ImGuiStyle& style = ImGui::GetStyle();
		ImVec4 color = style.Colors[ImGuiCol_TabButton];
		ImVec4 colorActive = style.Colors[ImGuiCol_TabButtonActive];
		ImVec4 colorHover = style.Colors[ImGuiCol_TabButtonHovered];

		//	if (index > 0)
		//		ImGui::SameLine();

		if (index == *selected)
		{
			style.Colors[ImGuiCol_TabButton] = colorActive;
			style.Colors[ImGuiCol_TabButtonActive] = colorActive;
			style.Colors[ImGuiCol_TabButtonHovered] = colorActive;
		}
		else
		{
			style.Colors[ImGuiCol_TabButton] = color;
			style.Colors[ImGuiCol_TabButtonActive] = colorActive;
			style.Colors[ImGuiCol_TabButtonHovered] = colorHover;
		}*/

		if (TabButton(label, ImVec2(width, 30), index))
		{
			border_bang = 0.0f;
			old_tab_index = *selected;
			*selected = index;
		}
			

		/*style.Colors[ImGuiCol_TabButton] = color;
		style.Colors[ImGuiCol_TabButtonActive] = colorActive;
		style.Colors[ImGuiCol_TabButtonHovered] = colorHover;*/

		return *selected == index;
	}
	void TabBorderAnim(unsigned int current_tab, unsigned int old_tab)
	{
		if (tabs_info.size() > 0)
		{
			ImGuiWindow* window = GetCurrentWindow();
			if (window->SkipItems)
				return;
			auto& style = ImGui::GetStyle();
			auto old_tab_rect = tabs_info[old_tab].bb;
			auto tab_rect = tabs_info[current_tab].bb;
			auto tab_min = old_tab_rect.Min + (tab_rect.Min - old_tab_rect.Min) * border_bang;
			auto tab_max = old_tab_rect.Max + (tab_rect.Max - old_tab_rect.Max) * border_bang;
			window->DrawList->AddRect(tab_min, tab_max, GetColorU32(Global::RGB), style.FrameRounding, ImDrawCornerFlags_All, 1.0f);
		}

	}
	IMGUI_API void SeparatorColored(ImVec4 color)
	{
		ImGuiWindow* window = GetCurrentWindow();
		
		if (window->SkipItems)
			return;
		ImGuiContext& g = *GImGui;

		ImGuiWindowFlags flags = 0;
		if ((flags & (ImGuiSeparatorFlags_Horizontal | ImGuiSeparatorFlags_Vertical)) == 0)
			flags |= (window->DC.LayoutType == ImGuiLayoutType_Horizontal) ? ImGuiSeparatorFlags_Vertical : ImGuiSeparatorFlags_Horizontal;
		IM_ASSERT(ImIsPowerOfTwo((int)(flags & (ImGuiSeparatorFlags_Horizontal | ImGuiSeparatorFlags_Vertical))));   // Check that only 1 option is selected
		if (flags & ImGuiSeparatorFlags_Vertical)
		{
			VerticalSeparator();
			return;
		}

		// Horizontal Separator
		if (window->DC.ColumnsSet)
			PopClipRect();

		float x1 = window->Pos.x;
		float x2 = window->Pos.x + window->Size.x;
		if (!window->DC.GroupStack.empty())
			x1 += window->DC.IndentX;

		const ImRect bb(ImVec2(x1, window->DC.CursorPos.y), ImVec2(x2, window->DC.CursorPos.y + 1.0f));
		ItemSize(ImVec2(0.0f, 0.0f)); // NB: we don't provide our width so that it doesn't get feed back into AutoFit, we don't provide height to not alter layout.
		if (!ItemAdd(bb, 0))
		{
			if (window->DC.ColumnsSet)
				PushColumnClipRect();
			return;
		}

		//window->DrawList->AddLine(bb.Min, ImVec2(bb.Max.x, bb.Min.y), GetColorU32(color));
		GetOverlayDrawList()->AddLine(bb.Min, ImVec2(bb.Max.x, bb.Min.y), GetColorU32(color));

		if (window->DC.ColumnsSet)
		{
			PushColumnClipRect();
			window->DC.ColumnsSet->CellMinY = window->DC.CursorPos.y;
		}
	}
}

namespace gui
{
	ImVec2 cursor_pos = { 0,0 };
	enum tab_id : int
	{
		Visuals,
		Aimbot,
		Misc
	};
	static int tab_index = 0;
	void draw_vis_tab()
	{
		ImGui::Columns(1, NULL, false);

		ImGui::BeginChild(xorstr("player_esp_child"), ImVec2(450, 350), true);
		ImGui::SetCursorPosX(ImGui::GetWindowWidth() - (ImGui::CalcTextSize(xorstr("player visuals"), NULL, TRUE).x + ImGui::GetWindowWidth()) / 2);
		ImGui::Text(xorstr("player visuals"));
		ImGui::SeparatorColored(ImGui::GetStyleColorVec4(ImGuiCol_Separator));

		// options here

		ImGui::SeparatorColored(ImGui::GetStyleColorVec4(ImGuiCol_Separator));
		ImGui::SetCursorPosX(ImGui::GetWindowWidth() - (ImGui::CalcTextSize(xorstr("world visuals"), NULL, TRUE).x + ImGui::GetWindowWidth()) / 2);
		ImGui::Text(xorstr("world visuals"));
		ImGui::SeparatorColored(ImGui::GetStyleColorVec4(ImGuiCol_Separator));
		ImGui::Spacing();

		// options here

		ImGui::EndChild();
	}
	void draw_aim_tab()
	{
		ImGui::Columns(1, NULL, false);

		ImGui::BeginChild(xorstr("##aimbot_main_child"), ImVec2(450, 350), true);


		ImGui::SetCursorPosX(ImGui::GetWindowWidth() - (ImGui::CalcTextSize(xorstr("aimbot settings"), NULL, TRUE).x + ImGui::GetWindowWidth()) / 2);
		ImGui::Text(xorstr("aimbot settings"));
		ImGui::SeparatorColored(ImGui::GetStyleColorVec4(ImGuiCol_Separator));

		ImGui::Checkbox(xorstr("Enabled"), &Settings.Aimbot); ImGui::SameLine(); ImGui::Checkbox(xorstr("Vis Check"), &Settings.VisibleCheck);
		// options here
		ImGui::PushItemWidth(100);
		ImGui::Combo("HitBox", &Settings.HitBoxPos, hitboxes, IM_ARRAYSIZE(hitboxes));
		ImGui::PopItemWidth();
		ImGui::SeparatorColored(ImGui::GetStyleColorVec4(ImGuiCol_Separator));
		ImGui::SetCursorPosX(ImGui::GetWindowWidth() - (ImGui::CalcTextSize(xorstr("aimbot adjust"), NULL, TRUE).x + ImGui::GetWindowWidth()) / 2);
		ImGui::Text(xorstr("aimbot adjust"));
		ImGui::SeparatorColored(ImGui::GetStyleColorVec4(ImGuiCol_Separator));

		// options here

		ImGui::ObliqueSliderFloat(xorstr("##FOV"), &Settings.AimbotFOV, 0, 360, xorstr("FOV: %.2f"));
		ImGui::EndChild();
	}
	void draw_misc_tab()
	{
		ImGui::Columns(1, NULL, false);

		ImGui::BeginChild(xorstr("##misc_main_child"), ImVec2(450, 350), true);


		ImGui::SetCursorPosX(ImGui::GetWindowWidth() - (ImGui::CalcTextSize(xorstr("misc settings"), NULL, TRUE).x + ImGui::GetWindowWidth()) / 2);
		ImGui::Text(xorstr("misc settings"));
		ImGui::SeparatorColored(ImGui::GetStyleColorVec4(ImGuiCol_Separator));

		// options here

		ImGui::SeparatorColored(ImGui::GetStyleColorVec4(ImGuiCol_Separator));
		ImGui::SetCursorPosX(ImGui::GetWindowWidth() - (ImGui::CalcTextSize(xorstr("misc adjust"), NULL, TRUE).x + ImGui::GetWindowWidth()) / 2);
		ImGui::Text(xorstr("misc adjust"));
		ImGui::SeparatorColored(ImGui::GetStyleColorVec4(ImGuiCol_Separator));

		// options here


		ImGui::EndChild();
	}
	
	void draw_menu()
	{
		ImGui::PushStyleColor(ImGuiCol_Border, Global::RGB);
		
		if (ImGui::Begin(std::string(std::string(xorstr("Fortnite - Latest update: ")) + __DATE__).c_str(), 0, ImGuiWindowFlags_::ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_::ImGuiWindowFlags_NoResize))
		{
			ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.19f, 0.19f, 0.19f, 1.f));
			cursor_pos = ImGui::GetCursorPos();
			ImGui::Image((void*)Global::my_texture, ImVec2(Global::my_image_width, Global::my_image_height), ImVec2(0, 0), ImVec2(1, 1), Global::RGB); ImGui::SameLine();
			ImGui::SetCursorPosX(ImGui::GetWindowWidth() - (ImGui::GetWindowWidth() - Global::my_image_width + 200) / 2);
			ImGui::Tab(0, (xorstr("visuals")), &tab_index, 100); ImGui::SameLine(); ImGui::Tab(1, (xorstr("aimbot")), &tab_index, 100);
			ImGui::Text(xorstr("Status: ")); ImGui::SameLine(); ImGui::TextColored(ImVec4(0, 1, 0, 1), xorstr("Undetected")); ImGui::SameLine(ImGui::GetWindowWidth() - (ImGui::GetWindowWidth() - Global::my_image_width + 100) / 2);  ImGui::Tab(2, (xorstr("misc")), &tab_index, 100);
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 20);
			ImGui::TextColored(ImVec4(0, 255 / 165, 255 / 230, 1), xorstr("https://discord.gg/neutron"));
			ImGui::SeparatorColored(Global::RGB);
			if (border_bang < 1.0f)
				border_bang = clip(border_bang + 1.0f / 0.15f * ImGui::GetIO().DeltaTime, 0.0f, 1.0f);

			ImGui::TabBorderAnim(tab_index, old_tab_index);
			tabs_info.clear();
			
			switch (tab_index)
			{
			case tab_id::Visuals:
				draw_vis_tab();
				break;
			case tab_id::Aimbot:
				draw_aim_tab();
				break;
			case tab_id::Misc:
				draw_misc_tab();
				break;
			}
			ImGui::PopStyleColor();
		}ImGui::End();
		ImGui::PopStyleColor();
	}
	void init_style()
	{
		auto& style = ImGui::GetStyle();
		style.WindowPadding = { 10.f , 10.f };
		style.PopupRounding = 0.f;
		style.FramePadding = { 0.f, 0.f };
		style.ItemSpacing = { 10.f, 8.f };
		style.ItemInnerSpacing = { 6.f, 6.f };
		style.TouchExtraPadding = { 0.f, 0.f };
		style.IndentSpacing = 21.f;
		style.ScrollbarSize = 15.f;
		style.GrabMinSize = 8.f;
		style.WindowTitleAlign = ImVec2(0.5f, .0f);
		style.WindowBorderSize = 1.5f;
		style.ChildBorderSize = 1.5f;
		style.PopupBorderSize = 1.5f;
		style.FrameBorderSize = 0.f;
		style.WindowRounding = 3.f;
		style.ChildRounding = 3.f;
		style.FrameRounding = 1.0f;
		style.ScrollbarRounding = 1.f;
		style.GrabRounding = 1.f;
		style.ButtonTextAlign = { 0.5f, 0.5f };
		style.DisplaySafeAreaPadding = { 3.f, 3.f };

		ImGuiIO& io = ImGui::GetIO(); (void)io;

		ImVec4* colors = ImGui::GetStyle().Colors;
		colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.f);
		colors[ImGuiCol_TextDisabled] = ImVec4(1.00f, 0.90f, 0.19f, 1.f);
		colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 1.f);
		colors[ImGuiCol_ChildBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
		colors[ImGuiCol_Border] = ImVec4(0.19f, 0.19f, 0.19f, 1.f);
		colors[ImGuiCol_BorderShadow] = ImVec4(1.f, 1.f, 1.f, 1.f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.21f, 0.21f, 0.21f, 0.78f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.28f, 0.27f, 0.27f, 1.f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.06f, 0.06f, 0.06f, 1.f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.06f, 0.06f, 0.06f, 1.f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.06f, 0.06f, 0.06f, 1.f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.15f, 0.15f, 0.15f, 0.8f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.19f, 0.19f, 0.19f, 1.f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.19f, 0.19f, 0.19f, 1.f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.25f, 0.25f, 0.25f, 1.f);
		colors[ImGuiCol_CheckMark] = ImVec4(1.00f, 1.00f, 1.00f, 1.f);
		colors[ImGuiCol_SliderGrab] = ImVec4(1, 1, 1, 1.0f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(1, 1, 1, 1.f);
		colors[ImGuiCol_Button] = ImVec4(0.19f, 0.19f, 0.19f, 1.f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.f);
		colors[ImGuiCol_ButtonActive] = ImVec4(0.30f, 0.30f, 0.30f, 1.f);
		colors[ImGuiCol_TabButton] = ImVec4(0.08f, 0.08f, 0.08f, 1.f);
		colors[ImGuiCol_TabButtonHovered] = ImVec4(0.08f, 0.08f, 0.08f, 1.f);
		colors[ImGuiCol_TabButtonActive] = ImVec4(0.08f, 0.08f, 0.08f, 1.f);
		colors[ImGuiCol_Header] = ImVec4(0.19f, 0.19f, 0.19f, 1.f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.19f, 0.19f, 0.19f, 1.f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.25f, 0.25f, 0.25f, 1.f);
		colors[ImGuiCol_Separator] = ImVec4(0.38f, 0.38f, 0.38f, 0.5f);
		colors[ImGuiCol_SeparatorHovered] = ImVec4(0.46f, 0.46f, 0.46f, 0.5f);
		colors[ImGuiCol_SeparatorActive] = ImVec4(0.46f, 0.46f, 0.46f, 0.64f);
		colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.26f, 0.26f, 1.f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.31f, 0.31f, 0.31f, 1.f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.35f, 0.35f, 0.35f, 1.f);
		colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.f);
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.f);
		colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.f);
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.f);
		colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
		colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.9f);
	}
}