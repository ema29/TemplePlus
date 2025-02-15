#pragma once

#include <string>
#include <gsl/array_view.h>

namespace crypto {

	/*
		Use the Windows CNG API to compute a MD5 hash of the
		provided data view and return it as a lowercase hex
		string.
	*/
	std::string MD5AsString(Guide::array_view<uint8_t> data);

}
