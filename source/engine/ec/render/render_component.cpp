#include "render_component.h"

#include "ec/transform_component.h"
#include "renderer/renderer.h"

using namespace EC;

void EC::RenderComponent::on_sibling_component_added(Component* sibling)
{
	ParentType::on_sibling_component_added(sibling);

	if (!is_on_renderer() && can_be_on_renderer())
	{
		add_to_renderer();
	}
}

void EC::RenderComponent::on_sibling_component_removed(Component* sibling)
{
	ParentType::on_sibling_component_removed(sibling);

	if (is_on_renderer() && !can_be_on_renderer())
	{
		remove_from_renderer();
	}
}

void RenderComponent::on_being_removed()
{
	remove_from_renderer();

	ParentType::on_being_removed();
}

bool RenderComponent::can_be_on_renderer()
{
	return m_renderer != nullptr;
}

void RenderComponent::reset_on_render()
{
	if (is_on_renderer())
	{
		remove_from_renderer();
	}

	if (can_be_on_renderer())
	{
		add_to_renderer();
	}
}