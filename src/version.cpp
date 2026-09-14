#include <string>
#include "mlReview/version.hpp"

using namespace MLReview;

int Version::getMajor() noexcept
{
    return MLReviewBackend_MAJOR;
}

int Version::getMinor() noexcept
{
    return MLReviewBackend_MINOR;
}

int Version::getPatch() noexcept
{
    return MLReviewBackend_PATCH;
}

//NOLINTBEGIN(bugprone-easily-swappable-parameters)
bool Version::isAtLeast(const int major, const int minor,
                        const int patch) noexcept
//NOLINTEND(bugprone-easily-swappable-parameters)
{
    if (MLReviewBackend_MAJOR < major){return false;}
    if (MLReviewBackend_MAJOR > major){return true;}
    if (MLReviewBackend_MINOR < minor){return false;}
    if (MLReviewBackend_MINOR > minor){return true;}
    if (MLReviewBackend_PATCH < patch){return false;}
    return true;
}

std::string Version::getVersion() noexcept
{
    std::string version{MLReviewBackend_VERSION};
    return version;
}

std::string Version::getTag() noexcept
{
    std::string tag{MLReviewBackend_GITTAG};
    return tag;
}

std::string Version::getVersionWithTag() noexcept
{
    auto tag = Version::getTag();
    if (tag.empty())
    {
        return Version::getVersion();
    }
    else
    {
        return Version::getVersion() + "-" + tag;
    }
}
