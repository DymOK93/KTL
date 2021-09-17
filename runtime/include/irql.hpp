#pragma once
#include <ntddk.h>

namespace ktl {
/**
 * @using using irql_t = KIRQL;
 * @brief C++-style type alias for IRQL values
 */
using irql_t = KIRQL;

/**
 * @fn irql_t get_current_irql();
 * @return Current Interrupt Request Level (IRQL)
 */
irql_t get_current_irql() noexcept;

/**
 * @fn irql_t raise_irql(irql_t new_irql);
 * @brief Raises the IRQL
 * @param[in] new_irql Desired IRQL, must be greater than or equal to the current one
 * @return Previous Interrupt Request Level (IRQL)
 */
irql_t raise_irql(irql_t new_irql) noexcept;

/**
 * @fn void lower_irql(irql_t new_irql);
 * @brief Raises the IRQL
 * @param[in] new_irql IRQL returned by the raise_irql()
 */
void lower_irql(irql_t new_irql) noexcept;

/**
 * @fn bool irql_less_or_equal(irql_t target_max_irql);
 * @brief Queries the current IRQL and compares with provided one
 * @param[in] target_max_irql Maximum expected IRQL
 * @return true, if the current IRQL <= target_max_irql 
 */
bool irql_less_or_equal(irql_t target_max_irql) noexcept;

/**
 * @fn bool irql_less_or_equal(irql_t target_max_irql);
 * @brief Calls KeBugCheck() if the current IRQL greater than provided one
 * @param[in] target_max_irql Maximum expected IRQL
 */
void verify_max_irql(irql_t target_max_irql) noexcept;
}  // namespace ktl