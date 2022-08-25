#pragma once

#ifndef _RESULT_H_
#define _RESULT_H_

#include <cstdlib>
#include <functional>
#include <type_traits>
#include <utility>
#include <variant>

#include "core.h"
#include "logger.h"

namespace core
{
	template <typename T>
	class Ok
	{
	public:
		template <typename OkType, typename ErrorType>
		friend class Result;

		Ok () = delete;
		// In place construction of an Ok value.
		template <class... _Args>
		explicit Ok (_Args&&... args): m_value (std::forward<_Args> (args)...)
		{
		}
		explicit Ok (Ok<T>&& other): m_value (std::move (other.m_value)) {}
		Ok<T>& operator= (const Ok<T>& other)
		{
			m_value = other;
			return *this;
		};
		Ok<T>& operator= (Ok<T>&& other) noexcept
		{
			m_value = std::move (other);
			return *this;
		};

		T& operator* () noexcept
		{
			return m_value;
		}

		T& operator-> () noexcept
		{
			return operator* ();
		}

	private:
		T m_value;
	};

	template <typename T>
	class Err
	{
	public:
		template <typename OkType, typename ErrorType>
		friend class Result;

		Err () = delete;
		// In place construction of an Err value.
		template <class... _Args>
		explicit Err (_Args&&... args): m_value (std::forward<_Args> (args)...)
		{
		}
		explicit Err (Err<T>&& other): m_value (std::move (other.m_value)) {}
		Err<T>& operator= (const Err<T>& other)
		{
			m_value = other;
			return *this;
		};
		Err<T>& operator= (Err<T>&& other) noexcept
		{
			m_value = std::move (other);
			return *this;
		};

		T& operator* () noexcept
		{
			return m_value;
		}

		T& operator-> () noexcept
		{
			return operator* ();
		}

	private:
		T m_value;
	};

	template <typename OkType, typename ErrorType>
	class Result
	{
	public:
		// Wapping the ok and error types with the Ok and Err
		// wrapper allows std::variant to see them as different types
		// allowing user code to have the same type for both the ok and
		// error case.
		using OkTy = Ok<OkType>;
		using ErrTy = Err<ErrorType>;

		Result () = delete;
		Result (OkTy&& ok): m_data (std::in_place_index_t<0> (), std::move (ok)) {}
		Result (ErrTy&& err): m_data (std::in_place_index_t<1> (), std::move (err)) {}
		/*
			returns true if the contained result was ok.
		*/
		constexpr bool isOk () const noexcept
		{
			return std::visit ([] (auto&& res) {
				using T = std::decay_t<decltype (res)>;
				return std::is_same<T, OkTy> ();
			});
		}

		/*
			returns true if the contained result was an error.
		*/
		constexpr bool isErr () const noexcept
		{
			return !isOk ();
		}

		// constexpr OkType unwrap ()
		// {
		// 	std::visit (
		// 		[] (auto&& res) {
		// 			using T = std::decay_t<decltype (res)>;
		// 			if constexpr (std::is_same<T, OkTy> ())
		// 			{
		// 				return std::move (*res);
		// 			}
		// 			else
		// 			{
		// 				LOG_CRITICAL ("called unwrap on an Err value");
		// 				FATAL_ERROR;
		// 			}
		// 		},
		// 		m_data);
		// }
		// constexpr ErrorType unwrapErr ()
		// {
		// 	std::visit (
		// 		[] (auto&& res) {
		// 			using T = std::decay_t<decltype (res)>;
		// 			if constexpr (std::is_same<T, ErrTy> ())
		// 			{
		// 				return std::move (*res);
		// 			}
		// 			else
		// 			{
		// 				LOG_CRITICAL ("called unwrapErr on an Ok value");
		// 				FATAL_ERROR;
		// 			}
		// 		},
		// 		m_data);
		// }

		/*
			Visit the result value with one of two passed in lambas
		*/
		void visit (std::function<void (OkTy&)> okVisitor, std::function<void (ErrTy&)> errorVisitor) noexcept
		{
			std::visit (
				[okVisitor, errorVisitor] (auto&& res) mutable {
					using T = std::decay_t<decltype (res)>;
					if constexpr (std::is_same<T, OkTy> ())
					{
						return okVisitor (res);
					}
					else if constexpr (std::is_same<T, ErrTy> ())
					{
						return errorVisitor (res);
					}
				},
				m_data);
		}

	private:
		std::variant<OkTy, ErrTy> m_data;
	};
} // namespace core
#endif // !_RESULT_H_B