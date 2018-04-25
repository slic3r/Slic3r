#ifndef MISC_UI_HPP
#define MISC_UI_HPP

/// Common static (that is, free-standing) functions, not part of an object hierarchy.

namespace Slic3r { namespace GUI {

/// Performs a check via the Internet for a new version of Slic3r.
/// If this version of Slic3r was compiled with SLIC3R_DEV, check the development 
/// space instead of release.
void check_version(bool manual = false);

}} // namespace Slic3r::GUI

#endif // MISC_UI_HPP
