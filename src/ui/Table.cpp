// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "Table.h"
#include "Context.h"
#include "Slider.h"

namespace UI {

void Table::LayoutAccumulator::AddRow(const std::vector<Widget*> &widgets)
{
	if (m_columnWidth.size() < widgets.size()) {
		std::size_t i = m_columnWidth.size();
		m_columnWidth.resize(widgets.size());
		m_columnLeft.resize(widgets.size());
		for (; i < widgets.size(); i++)
			m_columnWidth[i] = m_columnLeft[i] = 0;
	}

	m_columnLeft[0] = 0;
	for (std::size_t i = 0; i < widgets.size(); i++) {
		Widget *w = widgets[i];
		if (!w) continue;
		const Point size(w->CalcLayoutContribution());
		// XXX handle flags
		m_columnWidth[i] = std::max(m_columnWidth[i], size.x);
		if (i > 0) m_columnLeft[i] = m_columnLeft[i-1] + m_columnWidth[i-1] + m_columnSpacing;
	}
}

void Table::LayoutAccumulator::Clear()
{
	m_columnWidth.clear();
	m_columnLeft.clear();
}

void Table::LayoutAccumulator::SetColumnSpacing(int spacing) {
	m_columnSpacing = spacing;
	m_columnLeft.resize(m_columnWidth.size());
	if (m_columnLeft.size() > 0) {
		m_columnLeft[0] = 0;
		for (std::size_t i = 0; i < m_columnWidth.size(); i++)
			if (i > 0) m_columnLeft[i] = m_columnLeft[i-1] + m_columnWidth[i-1] + m_columnSpacing;
	}
}

Table::Inner::Inner(Context *context, LayoutAccumulator &layout) : Container(context),
	m_layout(layout),
	m_rowSpacing(0),
	m_rowAlignment(TOP),
	m_dirty(false),
	m_mouseEnabled(false)
{
}

Point Table::Inner::PreferredSize()
{
	if (!m_dirty)
		return m_preferredSize;

	const std::vector<int> &colWidth = m_layout.ColumnWidth();
	const std::vector<int> &colLeft = m_layout.ColumnLeft();
	if (colWidth.size() == 0) {
		m_preferredSize = Point();
		return m_preferredSize;
	}

	m_preferredSize.x = colLeft.back() + colWidth.back();
	m_preferredSize.y = 0;

	m_rowHeight.resize(m_rows.size());

	for (std::size_t i = 0; i < m_rows.size(); i++) {
		const std::vector<Widget*> &row = m_rows[i];
		m_rowHeight[i] = 0;
		for (std::size_t j = 0; j < row.size(); j++) {
			Widget *w = row[j];
			if (!w) continue;
			Point size(w->CalcLayoutContribution());
			m_rowHeight[i] = std::max(m_rowHeight[i], size.y);
		}
		m_preferredSize.y += m_rowHeight[i];
	}

	if (!m_rows.empty() && m_rowSpacing)
		m_preferredSize.y += (m_rows.size()-1)*m_rowSpacing;

	m_dirty = false;
	return m_preferredSize;
}

void Table::Inner::Layout()
{
	if (m_dirty)
		PreferredSize();

	int rowTop = 0;

	const std::vector<int> &colWidth = m_layout.ColumnWidth();
	const std::vector<int> &colLeft = m_layout.ColumnLeft();

	for (std::size_t i = 0; i < m_rows.size(); i++) {
		const std::vector<Widget*> &row = m_rows[i];
		for (std::size_t j = 0; j < row.size(); j++) {
			Widget *w = row[j];
			if (!w) continue;

			const Point preferredSize(w->PreferredSize());
			int height = std::min(preferredSize.y, m_rowHeight[i]);

			int off = 0;
			if (height != m_rowHeight[i]) {
				switch (m_rowAlignment) {
					case CENTER:
						off = (m_rowHeight[i] - height) / 2;
						break;
					case BOTTOM:
						off = m_rowHeight[i] - height;
						break;
					default:
						off = 0;
						break;
				}
			}

			SetWidgetDimensions(w, Point(colLeft[j], rowTop+off), Point(colWidth[j], height));
		}

		rowTop += m_rowHeight[i] + m_rowSpacing;
	}

	LayoutChildren();
}

void Table::Inner::Draw()
{
	int row_top, row_bottom;
	if (m_mouseEnabled && IsMouseOver() && RowUnderPoint(GetMousePos(), &row_top, &row_bottom) >= 0) {
		GetContext()->GetSkin().DrawRectHover(Point(0, row_top), Point(GetSize().x, row_bottom - row_top));
	}

	Container::Draw();
}

void Table::Inner::AddRow(const std::vector<Widget*> &widgets)
{
	m_rows.push_back(widgets);

	Point rowSize;
	for (std::size_t i = 0; i < widgets.size(); i++) {
		if (!widgets[i]) continue;
		AddWidget(widgets[i]);
	}

	m_dirty = true;
}

void Table::Inner::Clear()
{
	for (std::vector< std::vector<Widget*> >::const_iterator i = m_rows.begin(); i != m_rows.end(); ++i) {
		for (std::size_t j = 0; j < (*i).size(); j++) {
			if (!(*i)[j]) continue;
			RemoveWidget((*i)[j]);
		}
	}

	m_rows.clear();
	m_preferredSize = Point();

	m_dirty = false;
}

void Table::Inner::AccumulateLayout()
{
	for (std::vector< std::vector<Widget*> >::const_iterator i = m_rows.begin(); i != m_rows.end(); ++i)
		m_layout.AddRow(*i);

	m_dirty = true;
}

void Table::Inner::SetRowSpacing(int spacing)
{
	m_rowSpacing = spacing;
	m_dirty = true;
}

void Table::Inner::SetRowAlignment(RowAlignDirection dir)
{
	m_rowAlignment = dir;
	m_dirty = true;
}

void Table::Inner::HandleClick()
{
	if (m_mouseEnabled) {
		int row = RowUnderPoint(GetMousePos());
		if (row >= 0)
			onRowClicked.emit(row);
	}

	Container::HandleClick();
}

int Table::Inner::RowUnderPoint(const Point &pt, int *out_row_top, int *out_row_bottom) const
{
	int start = 0, end = m_rows.size()-1, mid = 0;
	while (start <= end) {
		mid = start+((end-start)/2);

		const Widget *w = m_rows[mid][0];
		const int rowTop = w->GetPosition().y;
		const int rowBottom = rowTop + w->GetSize().y;

		if (pt.y < rowTop)
			end = mid-1;
		else if (pt.y >= rowBottom)
			start = mid+1;
		else {
			if (out_row_top) { *out_row_top = rowTop; }
			if (out_row_bottom) { *out_row_bottom = rowBottom; }
			return mid;
		}
	}

	if (out_row_top) { *out_row_top = 0; }
	if (out_row_bottom) { *out_row_bottom = 0; }
	return -1;
}


Table::Table(Context *context) : Container(context),
	m_dirty(false)
{
	m_header.Reset(new Table::Inner(GetContext(), m_layout));
	AddWidget(m_header.Get());

	m_body.Reset(new Table::Inner(GetContext(), m_layout));
	AddWidget(m_body.Get());

	m_body->onRowClicked.connect(sigc::mem_fun(&onRowClicked, &sigc::signal<void,unsigned int>::emit));

	m_slider.Reset(GetContext()->VSlider());
	m_slider->onValueChanged.connect(sigc::mem_fun(this, &Table::OnScroll));
}

Point Table::PreferredSize()
{
	if (m_dirty) {
		m_layout.Clear();

		m_header->AccumulateLayout();
		m_body->AccumulateLayout();

		m_dirty = false;
	}

	const Point sliderSize = m_slider->PreferredSize();

	const Point headerPreferredSize = m_header->PreferredSize();
	const Point bodyPreferredSize = m_body->PreferredSize();

	return Point(std::max(headerPreferredSize.x,bodyPreferredSize.x)+sliderSize.x, headerPreferredSize.y+bodyPreferredSize.y);
}

void Table::Layout()
{
	if (m_dirty)
		PreferredSize();

	Point size = GetSize();

	Point preferredSize(m_header->PreferredSize());
	SetWidgetDimensions(m_header.Get(), Point(), Point(size.x, preferredSize.y));
	int top = preferredSize.y;
	size.y -= top;

	preferredSize = m_body->PreferredSize();
	if (preferredSize.y <= size.y) {
		if (m_slider->GetContainer()) {
			m_slider->SetValue(0);
			m_onMouseWheelConn.disconnect();
			RemoveWidget(m_slider.Get());
		}
	}
	else {
		AddWidget(m_slider.Get());
		if (!m_onMouseWheelConn.connected())
			m_onMouseWheelConn = onMouseWheel.connect(sigc::mem_fun(this, &Table::OnMouseWheel));

		const Point sliderSize(m_slider->PreferredSize().x, size.y);
		const Point sliderPos(size.x-sliderSize.x, top);
		SetWidgetDimensions(m_slider.Get(), sliderPos, sliderSize);

		size.x = sliderPos.x;
	}

	SetWidgetDimensions(m_body.Get(), Point(0, top), size);

	LayoutChildren();
}

void Table::HandleInvisible()
{
	m_slider->SetValue(0);
}

Table *Table::SetHeadingRow(const WidgetSet &set)
{
	m_header->Clear();
	m_header->AddRow(set.widgets);
	m_dirty = true;
	GetContext()->RequestLayout();
	return this;
}

Table *Table::AddRow(const WidgetSet &set)
{
	m_body->AddRow(set.widgets);
	m_dirty = true;
	GetContext()->RequestLayout();
	return this;
}

void Table::ClearRows()
{
	m_body->Clear();
	m_dirty = true;
	GetContext()->RequestLayout();
}

Table *Table::SetRowSpacing(int spacing)
{
	m_body->SetRowSpacing(spacing);
	m_dirty = true;
	GetContext()->RequestLayout();
	return this;
}

Table *Table::SetColumnSpacing(int spacing)
{
	m_layout.SetColumnSpacing(spacing);
	m_dirty = true;
	GetContext()->RequestLayout();
	return this;
}

Table *Table::SetRowAlignment(RowAlignDirection dir)
{
	m_body->SetRowAlignment(dir);
	m_dirty = true;
	GetContext()->RequestLayout();
	return this;
}

Table *Table::SetHeadingFont(Font font)
{
	m_header->SetFont(font);
	m_dirty = true;
	GetContext()->RequestLayout();
	return this;
}

Table *Table::SetMouseEnabled(bool enabled)
{
	m_body->SetMouseEnabled(enabled);
	return this;
}


void Table::OnScroll(float value)
{
	m_body->SetDrawOffset(Point(0, -float(m_body->PreferredSize().y-(GetSize().y-m_header->PreferredSize().y))*value));
}

bool Table::OnMouseWheel(const MouseWheelEvent &event)
{
	m_slider->SetValue(m_slider->GetValue() + (event.direction == MouseWheelEvent::WHEEL_UP ? -0.01f : 0.01f));
	return true;
}


}
