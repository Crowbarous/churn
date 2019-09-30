#include "prop.h"

namespace core
{

e_prop::e_prop ()
{
}

void e_prop::think ()
{
}

void e_prop::render ()
{
	glPushMatrix();

	glTranslatef(-pos.x, -pos.y, -pos.z);

	material->apply();
	model->render();

	glPopMatrix();
}

}