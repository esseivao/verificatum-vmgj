
/* Copyright 2008-2019 Douglas Wikstrom
 *
 * This file is part of Verificatum Multiplicative Groups library for
 * Java (VMGJ).
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

package com.verificatum.vmgj;

import com.sun.management.OperatingSystemMXBean;
import java.lang.management.ManagementFactory;
import java.math.BigInteger;
import java.util.concurrent.atomic.AtomicLong;

/**
 * Provides a Java wrapper for a pointer to a native pre-computed
 * table used for fixed based modular exponentiation as implemented in
 * {@link VMG}.
 *
 * @author Douglas Wikstrom
 */
public class FpowmTab implements AutoCloseable {

    /**
     * System property controlling the maximum estimated size of a
     * single fixed-base precomputation table in bytes.
     */
    static final String MAX_TABLE_BYTES_PROPERTY =
        "com.verificatum.vmgj.fpowm.maxTableBytes";

    /**
     * System property controlling the maximum estimated size of all
     * live fixed-base precomputation tables in bytes.
     */
    static final String MAX_TOTAL_BYTES_PROPERTY =
        "com.verificatum.vmgj.fpowm.maxTotalBytes";

    /**
     * Number of bytes in one gibibyte.
     */
    private static final long GIBIBYTE = 1024L * 1024L * 1024L;

    /**
     * Upper bound needed to safely evaluate 2^blockWidth using a long.
     */
    private static final int MAX_SUPPORTED_BLOCK_WIDTH = 24;

    /**
     * Absolute upper bound for the default aggregate table cap.
     */
    private static final long ABSOLUTE_MAX_TOTAL_BYTES = 8L * GIBIBYTE;

    /**
     * Absolute upper bound for the default per-table cap.
     */
    private static final long ABSOLUTE_MAX_TABLE_BYTES = 2L * GIBIBYTE;

    /**
     * Numerator of the fraction used for the default runtime total
     * memory cap.
     */
    private static final long RUNTIME_TOTAL_CAP_NUMERATOR = 1L;

    /**
     * Denominator of the fraction used for the default runtime total
     * memory cap.
     */
    private static final long RUNTIME_TOTAL_CAP_DENOMINATOR = 2L;

    /**
     * Numerator of the fraction used for the default runtime per-table
     * memory cap.
     */
    private static final long RUNTIME_TABLE_CAP_NUMERATOR = 1L;

    /**
     * Denominator of the fraction used for the default runtime
     * per-table memory cap.
     */
    private static final long RUNTIME_TABLE_CAP_DENOMINATOR = 4L;

    /**
     * Conservative native bookkeeping overhead per table.
     */
    private static final long TABLE_BASE_OVERHEAD_BYTES = 4096L;

    /**
     * Conservative native bookkeeping overhead per precomputed value.
     */
    private static final long ELEMENT_OVERHEAD_BYTES = 64L;

    /**
     * Tracks the sum of estimated bytes reserved for live tables.
     */
    private static final AtomicLong RESERVED_TABLE_BYTES =
        new AtomicLong(0L);

    /**
     * Stores native pointer to a precomputed fixed base
     * exponentiation table.
     */
    protected long tablePtr;

    /**
     * Estimated number of native bytes reserved for this table.
     */
    private long reservedBytes;

    /**
     * Estimates the native memory footprint of a fixed-base
     * precomputation table.
     *
     * @param modulus Modulus used during modular exponentiation.
     * @param blockWidth Number of basis elements used during
     * splitting.
     * @param exponentBitlen Expected bit length of exponents used when
     * invoking the table.
     * @return Conservative estimate of the native bytes consumed by
     * the precomputation table.
    *
    * <p>
    * The current estimate uses the modulus bit length and block
    * width. It does not currently scale with {@code exponentBitlen},
    * because the existing fixed-base precomputation shape in VMGJ is
    * dominated by the modulus-sized precomputed values.
     */
    public static long estimateTableBytes(final BigInteger modulus,
                                          final int blockWidth,
                                          final int exponentBitlen) {
        validateModulus(modulus);
        validateDimensions(blockWidth, exponentBitlen);

        final long modulusBytes = ceilDiv(modulus.bitLength(), 8L);
        final long precomputedValues = 1L << blockWidth;
        final long bytesPerValue = checkedAdd(checkedMultiply(modulusBytes, 2L),
                                              ELEMENT_OVERHEAD_BYTES);
        final long basisBytes = checkedMultiply((long) blockWidth, bytesPerValue);

        return checkedAdd(checkedAdd(checkedMultiply(precomputedValues,
                                                     bytesPerValue),
                                     basisBytes),
                          TABLE_BASE_OVERHEAD_BYTES);
    }

    /**
     * Returns the total estimated number of native bytes reserved for
     * live fixed-base precomputation tables in this JVM.
     *
     * @return Estimated number of native bytes reserved for live
     * tables.
     */
    public static long getReservedTableBytes() {
        return RESERVED_TABLE_BYTES.get();
    }

    /**
     * Creates a precomputed table for the given basis, modulus, and
     * exponent bit length.
     *
     * @param basis Basis element.
     * @param modulus Modulus used during modular exponentiations.
     * @param exponentBitlen Expected bit length of exponents used when
     * invoking the table.
     */
    public FpowmTab(final BigInteger basis,
                    final BigInteger modulus,
                    final int exponentBitlen) {
        this(basis, modulus, 16, exponentBitlen);
    }

    /**
     * Creates a precomputed table for the given basis, modulus, and
     * exponent bit length.
     *
     * @param basis Basis element.
     * @param modulus Modulus used during modular exponentiations.
     * @param blockWidth Number of basis elements used during
     * splitting.
     * @param exponentBitlen Expected bit length of exponents used when
     * invoking the table.
     */
    public FpowmTab(final BigInteger basis,
                    final BigInteger modulus,
                    final int blockWidth,
                    final int exponentBitlen) {
        if (basis == null) {
            throw new NullPointerException("basis");
        }

        validateModulus(modulus);
        validateDimensions(blockWidth, exponentBitlen);

        final long estimatedBytes =
            estimateTableBytes(modulus, blockWidth, exponentBitlen);

        reserveTableBytes(estimatedBytes);

        boolean success = false;
        try {
            tablePtr = VMG.fpowm_precomp(basis.toByteArray(),
                                         modulus.toByteArray(),
                                         blockWidth,
                                         exponentBitlen);
            if (tablePtr == 0) {
                throw new IllegalStateException("Native fpowm_precomp returned a null handle");
            }
            reservedBytes = estimatedBytes;
            success = true;
        } finally {
            if (!success) {
                releaseReservedTableBytes(estimatedBytes);
            }
        }
    }

    /**
     * Computes a modular exponentiation using the given exponent and
     * the basis and modulus previously used to construct this table.
     *
     * @param exponent Exponent used in modular exponentiation.
     * @return Power of basis for which pre-computation took place.
     */
    public synchronized BigInteger fpowm(final BigInteger exponent) {
        if (exponent == null) {
            throw new NullPointerException("exponent");
        }
        if (tablePtr == 0) {
            throw new IllegalStateException("FpowmTab has been freed");
        }
        return new BigInteger(VMG.fpowm(tablePtr,
                                        exponent.toByteArray()));
    }

    /**
     * Release resources allocated by native code.
     */
    public synchronized void free() {
        final long nativeTablePtr = tablePtr;
        final long tableBytes = reservedBytes;

        tablePtr = 0;
        reservedBytes = 0;

        try {
            if (nativeTablePtr != 0) {
                VMG.fpowm_clear(nativeTablePtr);
            }
        } finally {
            if (tableBytes != 0) {
                releaseReservedTableBytes(tableBytes);
            }
        }
    }

    /**
     * Releases resources allocated by native code.
     */
    @Override
    public void close() {
        free();
    }

    /**
     * Validates the modulus.
     *
     * @param modulus Modulus used during modular exponentiation.
     */
    private static void validateModulus(final BigInteger modulus) {
        if (modulus == null) {
            throw new NullPointerException("modulus");
        }
        if (modulus.signum() <= 0) {
            throw new IllegalArgumentException("modulus must be positive");
        }
        if (modulus.bitLength() == 0) {
            throw new IllegalArgumentException("modulus must have positive bit length");
        }
    }

    /**
     * Validates parameters controlling the size of a precomputation.
     *
     * @param blockWidth Width used for the precomputation.
     * @param exponentBitlen Bit length used for the precomputation.
     */
    private static void validateDimensions(final int blockWidth,
                                           final int exponentBitlen) {
        if (blockWidth <= 0) {
            throw new IllegalArgumentException("blockWidth must be positive");
        }
        if (blockWidth > MAX_SUPPORTED_BLOCK_WIDTH) {
            throw new IllegalArgumentException("blockWidth exceeds supported maximum of "
                                               + MAX_SUPPORTED_BLOCK_WIDTH);
        }
        if (exponentBitlen <= 0) {
            throw new IllegalArgumentException("exponentBitlen must be positive");
        }
    }

    /**
     * Reserves an estimated number of bytes for a new table.
     *
     * @param estimatedBytes Estimated native bytes for the table.
     */
    private static void reserveTableBytes(final long estimatedBytes) {
        final long totalPhysicalMemory = detectTotalPhysicalMemoryBytes();
        final long defaultTotalBytes = getDefaultTotalCapBytes(totalPhysicalMemory);
        final long defaultTableBytes =
            getDefaultTableCapBytes(totalPhysicalMemory, defaultTotalBytes);

        final long maxTotalBytes =
            getConfiguredLimit(MAX_TOTAL_BYTES_PROPERTY,
                               defaultTotalBytes);
        final long maxTableBytes =
            getConfiguredLimit(MAX_TABLE_BYTES_PROPERTY,
                               defaultTableBytes);
        final long effectiveMaxTableBytes = Math.min(maxTableBytes, maxTotalBytes);

        if (estimatedBytes > effectiveMaxTableBytes) {
            throw new IllegalArgumentException("Estimated fixed-base table size "
                                               + estimatedBytes
                                               + " bytes exceeds limit "
                                               + effectiveMaxTableBytes
                                               + " bytes. Lower blockWidth or set -D"
                                               + MAX_TABLE_BYTES_PROPERTY
                                               + "=<bytes>");
        }

        while (true) {
            final long reserved = RESERVED_TABLE_BYTES.get();
            final long updated = checkedAdd(reserved, estimatedBytes);

            if (updated > maxTotalBytes) {
                throw new IllegalStateException("Estimated total size of live fixed-base tables "
                                                + updated
                                                + " bytes exceeds limit "
                                                + maxTotalBytes
                                                + " bytes. Free unused tables or set -D"
                                                + MAX_TOTAL_BYTES_PROPERTY
                                                + "=<bytes>");
            }

            if (RESERVED_TABLE_BYTES.compareAndSet(reserved, updated)) {
                return;
            }
        }
    }

    /**
     * Releases an estimated number of reserved native bytes.
     *
     * @param estimatedBytes Estimated native bytes for the table.
     */
    private static void releaseReservedTableBytes(final long estimatedBytes) {
        while (true) {
            final long reserved = RESERVED_TABLE_BYTES.get();
            final long updated = reserved - estimatedBytes;

            if (updated < 0) {
                RESERVED_TABLE_BYTES.compareAndSet(reserved, 0);
                return;
            }
            if (RESERVED_TABLE_BYTES.compareAndSet(reserved, updated)) {
                return;
            }
        }
    }

    /**
     * Parses a configured native-memory limit.
     *
     * @param propertyName Property containing the configured limit.
     * @param defaultValue Default value used when no property is set.
     * @return Configured limit in bytes.
     */
    private static long getConfiguredLimit(final String propertyName,
                                           final long defaultValue) {
        final String value = System.getProperty(propertyName);

        if (value == null || value.length() == 0) {
            return defaultValue;
        }

        try {
            final long parsed = Long.parseLong(value);

            if (parsed <= 0) {
                return Long.MAX_VALUE;
            }
            return parsed;
        } catch (NumberFormatException nfe) {
            throw new IllegalArgumentException("Failed to parse system property "
                                               + propertyName
                                               + " as a byte count", nfe);
        }
    }

    /**
     * Returns the default aggregate cap for all live fixed-base
     * precomputation tables.
     *
     * @param totalPhysicalMemory Detected total physical memory, or
     * a non-positive value if unavailable.
     * @return Default aggregate cap in bytes.
     */
    private static long getDefaultTotalCapBytes(final long totalPhysicalMemory) {
        if (totalPhysicalMemory <= 0) {
            return ABSOLUTE_MAX_TOTAL_BYTES;
        }

        final long runtimeCap = totalPhysicalMemory
            * RUNTIME_TOTAL_CAP_NUMERATOR
            / RUNTIME_TOTAL_CAP_DENOMINATOR;
        return Math.min(runtimeCap, ABSOLUTE_MAX_TOTAL_BYTES);
    }

    /**
     * Returns the default cap for a single fixed-base precomputation
     * table.
     *
     * @param totalPhysicalMemory Detected total physical memory, or
     * a non-positive value if unavailable.
     * @param defaultTotalBytes Default aggregate cap in bytes.
     * @return Default per-table cap in bytes.
     */
    private static long getDefaultTableCapBytes(final long totalPhysicalMemory,
                                                final long defaultTotalBytes) {
        if (totalPhysicalMemory <= 0) {
            return Math.min(ABSOLUTE_MAX_TABLE_BYTES, defaultTotalBytes);
        }

        final long runtimeTableCap = totalPhysicalMemory
            * RUNTIME_TABLE_CAP_NUMERATOR
            / RUNTIME_TABLE_CAP_DENOMINATOR;
        return Math.min(Math.min(runtimeTableCap, ABSOLUTE_MAX_TABLE_BYTES),
                        defaultTotalBytes);
    }

    /**
     * Best-effort detection of total physical memory from the JVM
     * operating-system bean.
     *
     * <p>
     * This method is advisory only and is not used as a correctness
     * requirement. If unavailable, VMGJ falls back to configured
     * limits only.
     *
     * @return Detected total physical memory in bytes, or {@code -1}
     * if unavailable.
     */
    private static long detectTotalPhysicalMemoryBytes() {
        try {
            final java.lang.management.OperatingSystemMXBean osBean =
                ManagementFactory.getOperatingSystemMXBean();

            if (osBean instanceof OperatingSystemMXBean) {
                final long total =
                    ((OperatingSystemMXBean) osBean)
                    .getTotalMemorySize();
                if (total > 0) {
                    return total;
                }
            }
            return -1;
        } catch (RuntimeException re) {
            return -1;
        }
    }

    /**
     * Computes ceil(dividend / divisor).
     *
     * @param dividend Number to divide.
     * @param divisor Number used to divide.
     * @return Ceiling of the quotient.
     */
    private static long ceilDiv(final long dividend, final long divisor) {
        return (dividend + divisor - 1L) / divisor;
    }

    /**
     * Multiplies two positive values while checking for overflow.
     *
     * @param left First factor.
     * @param right Second factor.
     * @return Product of the inputs.
     */
    private static long checkedMultiply(final long left, final long right) {
        if (left == 0 || right == 0) {
            return 0;
        }
        if (left > Long.MAX_VALUE / right) {
            throw new IllegalArgumentException("Requested fixed-base table is too large to estimate safely");
        }
        return left * right;
    }

    /**
     * Adds two positive values while checking for overflow.
     *
     * @param left First addend.
     * @param right Second addend.
     * @return Sum of the inputs.
     */
    private static long checkedAdd(final long left, final long right) {
        if (left > Long.MAX_VALUE - right) {
            throw new IllegalArgumentException("Requested fixed-base table is too large to estimate safely");
        }
        return left + right;
    }

    // It took them about 20 years to figure out that finalize() without
    // any guarantees is harmful.
    //
    // /**
    //  * This is optimistic, but we only allocate a fixed amount of
    //  * memory and do not rely on this.
    //  *
    //  * @throws Throwable If this instance can not be finalized.
    //  */
    // protected void finalize() throws Throwable {
    //     free();
    //     super.finalize();
    // }
}
