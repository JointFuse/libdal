#ifndef DALCORE_H
#define DALCORE_H

#include <memory>

#define DAL_DECLARE_PIMPL \
class _impl; std::unique_ptr<_impl> pimpl;
#define DAL_PIMPL_DEFAULT_CONSTRUCTOR(ClassName) \
ClassName::ClassName() : pimpl{ std::make_unique<_impl>() } {}
#define DAL_PIMPL_THIS_CONSTRUCTOR(ClassName) \
ClassName::ClassName() : pimpl{ std::make_unique<_impl>(this) } {}
#define DAL_PIMPL_DEFAULT_DESTRUCTOR(ClassName) \
ClassName::~ClassName() = default;

#endif // DALCORE_H
