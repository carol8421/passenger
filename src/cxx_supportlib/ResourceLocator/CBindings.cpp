#include <ResourceLocator/Locator.h>
#include <ResourceLocator/CBindings.h>
#include <cstring>

using namespace Passenger;


PsgResourceLocator *
psg_resource_locator_new(const char *installSpec, size_t len,
	PP_Error *error)
{
	if (len == (size_t) -1) {
		len = strlen(installSpec);
	}

	try {
		return new ResourceLocator(std::string(installSpec, len));
	} catch (const std::exception &e) {
		pp_error_set(e, error);
		return NULL;
	}
}

void
psg_resource_locator_free(PsgResourceLocator *locator) {
	ResourceLocator *cxxLocator = static_cast<ResourceLocator *>(locator);
	delete cxxLocator;
}
