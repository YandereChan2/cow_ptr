#pragma once

#include <memory>

namespace Yc
{
    namespace details
    {
        template<class T>
        struct locally_ctrl_block
        {
            size_t ref_cnt{};
            union u_t
            {
                T t;
                ~u_t() {}
            }u;
        };
        constexpr inline struct no_allocator_arg_t
        {
            explicit no_allocator_arg_t() = default;
        }no_allocator_arg;
    }

    

    template<class T, class Allocator = std::allocator<T>>
    class locally_cow_ptr
    {
        using ctrl_block_alloc = std::allocator_traits<Allocator>::template rebind_alloc<details::locally_ctrl_block<T>>;
        using ctrl_block_traits = std::allocator_traits<Allocator>::template rebind_traits<details::locally_ctrl_block<T>>;

        using block_ptr = ctrl_block_traits::pointer;
        [[no_unique_address]] Allocator alloc{};
        block_ptr ptr;

        block_ptr allocate_block()
        {
            ctrl_block_alloc a{ alloc };
            return ctrl_block_traits::allocate(a, 1);
        }

        void deallocate_block(block_ptr ptr)noexcept
        {
            ctrl_block_alloc a{ alloc };
            ctrl_block_traits::deallocate(a, ptr, 1);
        }

        

        void dec_ref()noexcept
        {
            --ptr->ref_cnt;
            if (ptr->ref_cnt == 0)
            {
                std::allocator_traits<Allocator>::destroy(alloc, std::addressof(ptr->u.t));
                deallocate_block(ptr);
            }
        }
        template<class... Args>
        locally_cow_ptr(details::no_allocator_arg_t, Args&&... args):ptr{ allocate_block()}
        {
            struct _guard
            {
                locally_cow_ptr* ptr;
                block_ptr blk;
                ~_guard()
                {
                    if (ptr)
                    {
                        ptr->deallocate_block(blk);
                    }
                }
            }grd{this, ptr};
            std::allocator_traits<Allocator>::construct(alloc, std::addressof(ptr->u.t), std::forward<Args>(args)...);
            ptr->ref_cnt = 1;
            grd.ptr = nullptr;
        }
    public:
        using value_type = T;
        using allocator_type = Allocator;
        locally_cow_ptr(std::nullptr_t = nullptr) = delete;
        // 这个构造函数不建议使用，暴露于此仅为了和分配器有关的框架使用
        template<class... Args>
        locally_cow_ptr(std::allocator_arg_t, const Allocator& alloc, Args&&... args) :alloc{ alloc }, ptr{ allocate_block() }
        {
            struct _guard
            {
                locally_cow_ptr* ptr;
                block_ptr blk;
                ~_guard()
                {
                    if (ptr)
                    {
                        ptr->deallocate_block(blk);
                    }
                }
            }grd{ this, ptr };
            std::allocator_traits<Allocator>::construct(alloc, std::addressof(ptr->u.t), std::forward<Args>(args)...);
            ptr->ref_cnt = 1;
            grd.ptr = nullptr;
        }

        locally_cow_ptr(const locally_cow_ptr& other)noexcept :alloc{other.alloc},ptr { other.ptr }
        {
            ++ptr->ref_cnt;
        }

        locally_cow_ptr& operator=(const locally_cow_ptr& other)noexcept
        {
            if (this == std::addressof(other))
            {
                return *this;
            }

            using traits = std::allocator_traits<Allocator>;
            if constexpr (traits::propagate_on_container_copy_assignment::value)
            {
                dec_ref();
                alloc = other.alloc;
                ptr = other.ptr;
                ++ptr->ref_cnt;
                
            }
            else
            {
                if (alloc == other.alloc && ptr != other.ptr)
                {
                    dec_ref();
                    ptr = other.ptr;
                    ++ptr->ref_cnt;
                }
                else
                {
                    if (unique())
                    {
                        ptr->u.t = other.const_value();
                    }
                    else
                    {
                        block_ptr new_block = allocate_block();
                        struct _guard
                        {
                            locally_cow_ptr* ptr;
                            block_ptr blk;
                            ~_guard()
                            {
                                if (ptr)
                                {
                                    ptr->deallocate_block(blk);
                                }
                            }
                        }grd{ this, new_block };

                        std::allocator_traits<Allocator>::construct(alloc, std::addressof(new_block->u.t), other.const_value());
                        --ptr->ref_cnt;
                        ptr = new_block;
                        new_block->ref_cnt = 1;
                        grd.ptr = nullptr;
                    }
                }
            }
            return *this;
        }

        ~locally_cow_ptr()
        {
            dec_ref();
        }

        bool unique()const noexcept
        {
            return ptr->ref_cnt == 1;
        }

        size_t use_count()const noexcept
        {
            return ptr->ref_cnt;
        }

        const T& const_value()const noexcept
        {
            return ptr->u.t;
        }

        T& value()
        {
            if (!unique())
            {
                block_ptr new_block = allocate_block();
                struct _guard
                {
                    locally_cow_ptr* ptr;
                    block_ptr blk;
                    ~_guard()
                    {
                        if (ptr)
                        {
                            ptr->deallocate_block(blk);
                        }
                    }
                }grd{this, new_block};

                std::allocator_traits<Allocator>::construct(alloc, std::addressof(new_block->u.t), ptr->u.t);

                --ptr->ref_cnt;
                ptr = new_block;
                new_block->ref_cnt = 1;
                grd.ptr = nullptr;
            }
            return ptr->u.t;
        }

        const T& value()const noexcept
        {
            return const_value();
        }

        // 使用以下运算符重载时记得使用std::as_const
        T& operator*()
        {
            return value();
        }

        const T& operator*()const noexcept
        {
            return const_value();
        }

        T* operator->()
        {
            return std::addressof(value());
        }

        const T* operator->()const noexcept
        {
            return std::addressof(const_value());
        }

        void swap(locally_cow_ptr& other)noexcept
        {
            using std::swap;
            swap(alloc, other.alloc);
            std::swap(ptr, other.ptr);
        }

        template<class T, class Allocator, class... Args >
        friend locally_cow_ptr<T, Allocator> make_locally_cow_ptr(Args&&... args);

        template<class T, class Allocator, class... Args>
        friend locally_cow_ptr<T, Allocator> make_locally_cow_ptr_with_allocator(const Allocator& alloc, Args&&... args);
    };

    // 推荐使用这两个工厂函数
    template<class T, class Allocator = std::allocator<T>, class... Args >
    locally_cow_ptr<T, Allocator> make_locally_cow_ptr(Args&&... args)
    {
        return { details::no_allocator_arg, std::forward<Args>(args)... };
    }

    template<class T, class Allocator, class... Args>
    locally_cow_ptr<T, Allocator> make_locally_cow_ptr_with_allocator(const Allocator& alloc,Args&&... args)
    {
        return { std::allocator_arg, alloc, std::forward<Args>(args)... };
    }

    template<class T, class Allocator>
    void swap(locally_cow_ptr<T, Allocator>& l, locally_cow_ptr<T, Allocator>& r)noexcept
    {
        l.swap(r);
    }
}

