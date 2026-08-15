UINT64 <- "uint64"

setOldClass(UINT64)

#' Create a new uint64 vector.
#' @param length vector length
#' @export
uint64 <- function(length=0) {
  ret <- double(length)
  oldClass(ret) <- UINT64
  return(ret)
}

#' Check if the object x is of uint64 class.
#' @param x object
#' @export
is.uint64 <- function(x) {
  return(inherits(x, UINT64))
}

#' Compare two uint64 vectors for exact equality.
#'
#' identical() is not an S3 generic, so this function has to be called
#' explicitly; use all(x == y) as an alternative. num.eq defaults to FALSE so
#' that the comparison is bitwise rather than numeric, which is required because
#' a uint64 is the raw 64 bit pattern reinterpreted as a double.
#' @param x uint64 vector
#' @param y uint64 vector
#' @param num.eq compare numbers bitwise rather than numerically
#' @param single.NA treat all NaN bit patterns as one value
#' @param attrib.as.set compare attributes as an unordered set
#' @param ignore.bytecode ignore byte code state
#' @export
identical.uint64 <- function(x, y, num.eq=FALSE, single.NA=FALSE, attrib.as.set=TRUE, ignore.bytecode=TRUE) {
  return(identical(x=x, y=y, num.eq=num.eq, single.NA=single.NA, attrib.as.set=attrib.as.set, ignore.bytecode=ignore.bytecode))
}

#' Coerce to uint64.
#' @param x vector
#' @export
as.uint64 <- function(x, ...) {
  UseMethod("as.uint64")
}

#' Coerce from factor to uint64.
#'
#' The factor is converted through its labels, because unclass() on a factor
#' yields the integer codes rather than the values it holds.
#' @param x factor vector
#' @export
as.uint64.factor <- function(x, ...) {
  return(as.uint64(as.character(x), ...))
}

#' Coerce from NULL to uint64.
#' @param x NULL vector
#' @export
as.uint64.NULL <- function(x, ...) {
  return(uint64())
}

#' Coerce from uint64 to uint64.
#' @param x uint64 vector
#' @export
as.uint64.uint64 <- function(x, ...) {
  return(x)
}

#' Coerce double vector to uint64
#' @param x double vector
#' @useDynLib variantkey R_double_to_uint64
#' @export
as.uint64.double <- function(x, ...) {
  ret <- uint64(length(x))
  return(.Call("R_double_to_uint64", x, ret))
}

#' Coerce integer vector to uint64
#' @param x integer vector
#' @useDynLib variantkey R_integer_to_uint64
#' @export
as.uint64.integer <- function(x, ...) {
  ret <- uint64(length(x))
  return(.Call("R_integer_to_uint64", x, ret))
}

#' Coerce character vector to uint64
#' @param x character vector
#' @useDynLib variantkey R_decstr_to_uint64
#' @export
as.uint64.character <- function(x, ...) {
  ret <- uint64(length(x))
  return(.Call("R_decstr_to_uint64", x, ret))
}

setAs("character", UINT64, function(from)as.uint64.character(from))

#' Coerce uint64 vector to character
#' @param x uint64 vector
#' @useDynLib variantkey R_uint64_to_decstr
#' @export
as.character.uint64 <- function(x, ...) {
  ret <- character(length(x))
  return(.Call("R_uint64_to_decstr", x, ret))
}

setAs(UINT64, "character", function(from)as.character.uint64(from))

#' Convert hexadecimal character vector to uint64.
#' @param x hexadecimal character vector (16 characters per item)
#' @useDynLib variantkey R_parse_hex_uint64_t
#' @export
as.uint64.hex64 <- function(x, ...) {
  ret <- uint64(length(x))
  return(.Call("R_parse_hex_uint64_t", as.character(x), ret))
}

#' Replicate elements of uint64 vectors.
#' @param x uint64 vector to be replicated
#' @export
"rep.uint64" <- function(x, ...) {
  cx <- oldClass(x)
  ret <- NextMethod()
  oldClass(ret) <- cx
  return(ret)
}

#' Set the length of uint64 vector.
#' @param x uint64 vector
#' @param value value to set the new length
#' @export
"length<-.uint64" <- function(x, value) {
  cx <- oldClass(x)
  n <- length(x)
  x <- NextMethod()
  oldClass(x) <- cx
  if (value > n) {
    x[(n + 1):value] <- as.uint64(0)
  }
  return(x)
}

#' Extract uint64 vector parts
#' @param x uint64 vector
#' @export
"[.uint64" <- function(x,...) {
  cx <- oldClass(x)
  ret <- NextMethod()
  oldClass(ret) <- cx
  return(ret)
}

#' Extract uint64 vector parts
#' @param x uint64 vector
#' @export
"[[.uint64" <- function(x,...) {
  cx <- oldClass(x)
  ret <- NextMethod()
  oldClass(ret) <- cx
  return(ret)
}

#' Replace parts of uint64 vector
#' @param x uint64 vector
#' @param value uint64 replacement value
#' @export
"[<-.uint64" <- function(x,...,value) {
  cx <- oldClass(x)
  value <- as.uint64(value)
  ret <- NextMethod()
  oldClass(ret) <- cx
  return(ret)
}

#' Replace parts of uint64 vector 
#' @param x uint64 vector
#' @param value uint64 replacement value
#' @export
"[[<-.uint64" <- function(x,...,value) {
  cx <- oldClass(x)
  value <- as.uint64(value)
  ret <- NextMethod()
  oldClass(ret) <- cx
  return(ret)
}

# Length of the result of a comparison. As in base R, an operation with a
# zero-length operand returns a zero-length result: there is nothing to recycle.
vk_cmp_length <- function(x, y) {
  if ((length(x) == 0) || (length(y) == 0)) {
    return(0)
  }
  return(max(length(x), length(y)))
}

#' Returns true if x and y are equal.
#' @param x uint64 vector
#' @param y uint64 vector
#' @useDynLib variantkey R_EQ_uint64
#' @export
"==.uint64" <- function(x, y) {
  ret <- logical(vk_cmp_length(x, y))
  return(.Call("R_EQ_uint64", as.uint64(x), as.uint64(y), ret))
}

#' Returns true if x and y are different.
#' @param x uint64 vector
#' @param y uint64 vector
#' @useDynLib variantkey R_NE_uint64
#' @export
"!=.uint64" <- function(x, y) {
  ret <- logical(vk_cmp_length(x, y))
  return(.Call("R_NE_uint64", as.uint64(x), as.uint64(y), ret))
}

#' Returns true if x is less than y.
#' @param x uint64 vector
#' @param y uint64 vector
#' @useDynLib variantkey R_LT_uint64
#' @export
"<.uint64" <- function(x, y) {
  ret <- logical(vk_cmp_length(x, y))
  return(.Call("R_LT_uint64", as.uint64(x), as.uint64(y), ret))
}

#' Returns true if x is less or equal than y.
#' @param x uint64 vector
#' @param y uint64 vector
#' @useDynLib variantkey R_LE_uint64
#' @export
"<=.uint64" <- function(x, y) {
  ret <- logical(vk_cmp_length(x, y))
  return(.Call("R_LE_uint64", as.uint64(x), as.uint64(y), ret))
}

#' Returns true if x is greater than y.
#' @param x uint64 vector
#' @param y uint64 vector
#' @useDynLib variantkey R_GT_uint64
#' @export
">.uint64" <- function(x, y) {
  ret <- logical(vk_cmp_length(x, y))
  return(.Call("R_GT_uint64", as.uint64(x), as.uint64(y), ret))
}

#' Returns true if x is greater or equal than y.
#' @param x uint64 vector
#' @param y uint64 vector
#' @useDynLib variantkey R_GE_uint64
#' @export
">=.uint64" <- function(x, y) {
  ret <- logical(vk_cmp_length(x, y))
  return(.Call("R_GE_uint64", as.uint64(x), as.uint64(y), ret))
}

#' Format uint64 vector for pretty printing.
#' @export
format.uint64 <- function(x, ...) {
  return(format(as.character(x), ...))
}

#' Prints uint64 argument and returns it invisibly.
#' @export
print.uint64 <- function(x, ...) {
  return(print(as.character(x), ...))
}

bindUint64 <- function(mode, recursive=FALSE, ...) {
  x <- list(...)
  n <- length(x)
  for (i in 1:n) {
    if (recursive && is.list(x[[i]])) {
      x[[i]] <- do.call("c.uint64", c(x[[i]], list(recursive=TRUE)))
    } else {
      if (!is.uint64(x[[i]])) {
        m <- names(x[[i]])
        x[[i]] <- as.uint64(x[[i]])
        names(x[[i]]) <- m
      }
      oldClass(x[[i]]) <- NULL
    }
  }
  ret <- do.call(mode, x)
  oldClass(ret) <- UINT64
  return(ret)
}

#' Concatenate uint64 vectors.
#' @param Two or more vectors coerced to uint64
#' @export
c.uint64 <- function(..., recursive=FALSE) {
  return(bindUint64(mode="c", recursive=recursive, ...))
}

#' Combine uint64 vectors by columns.
#' @export
cbind.uint64 <- function(...) {
  return(bindUint64(mode="cbind", recursive=FALSE, ...))
}

#' Combine uint64 vectors by rows.
#' @export
rbind.uint64 <- function(...) {
  return(bindUint64(mode="rbind", recursive=FALSE, ...))
}

remUint64Class <- function(x) {
  if (length(x)) {
    i <- (x == UINT64)
    if (any(i)) {
      return(x[!i])
    }
  }
  return(x)
}

#' Coerce uint64 vector to data.frame.
#' @param x uint64 vector
#' @export
as.data.frame.uint64 <- function(x, ...) {
  # class<- is used instead of setattr(), which is not in this package's Imports.
  cx <- oldClass(x)
  y <- x
  class(y) <- remUint64Class(cx)
  ret <- as.data.frame(y, ...)
  n <- length(ret)
  for (i in seq_len(n)) {
    class(ret[[i]]) <- cx
  }
  return(ret)
}

#' Sorts a uint64 vector in ascending order.
#' @param x uint64 vector
#' @useDynLib variantkey R_sort_uint64
#' @export
sort.uint64 <- function(x, decreasing = FALSE, ...) {
  n <- length(x)
  tmp <- uint64(n)
  ret <- uint64(n)
  ret <- .Call("R_sort_uint64", as.uint64(x), tmp, ret)
  if (isTRUE(decreasing)) {
    ret <- rev.uint64(ret)
  }
  return(ret)
}

#' Returns a permutation which rearranges its first argument into ascending order.
#' @param x uint64 vector
#' @useDynLib variantkey R_order_uint64
#' @export
order.uint64 <- function(x) {
  n <- length(x)
  tmp <- uint64(n)
  ret <- uint64(n)
  idx <- integer(n)
  tdx <- integer(n)
  return(.Call("R_order_uint64", as.uint64(x), tmp, ret, idx, tdx) + 1)
}

#' Reverse a uint64 vector.
#' @param x uint64 vector
#' @useDynLib variantkey R_reverse_uint64
#' @export
rev.uint64 <- function(x, ...) {
  ret <- uint64(length(x))
  return(.Call("R_reverse_uint64", as.uint64(x), ret))
}

#' Removes duplicated values from a uint64 vector.
#'
#' The vector is sorted first, because the underlying C unique_uint64_t() only
#' collapses consecutive runs of equal values. The result is in ascending order.
#' @param x uint64 vector
#' @useDynLib variantkey R_unique_uint64
#' @export
unique.uint64 <- function(x, ...) {
  x <- sort.uint64(as.uint64(x))
  ret <- uint64(length(x))
  return(.Call("R_unique_uint64", x, ret))
}

#' Returns the intersection of two sorted uint64 vectors.
#' @param x uint64 vector
#' @param y uint64 vector
#' @useDynLib variantkey R_intersect_uint64
#' @export
intersect.uint64 <- function(x, y) {
  ret <- uint64(min(length(x), length(y)))
  return(.Call("R_intersect_uint64", as.uint64(x), as.uint64(y), ret))
}

#' Returns the union of two sorted uint64 vectors.
#' @param x uint64 vector
#' @param y uint64 vector
#' @useDynLib variantkey R_union_uint64
#' @export
union.uint64 <- function(x, y) {
  ret <- uint64(length(x) + length(y))
  return(.Call("R_union_uint64", as.uint64(x), as.uint64(y), ret))
}

#' Comparison key for a uint64 vector.
#'
#' Provides the ordering used by order(), sort.list() and rank(), which would
#' otherwise compare the underlying doubles, i.e. the raw 64 bit patterns.
#' @param x uint64 vector
#' @export
xtfrm.uint64 <- function(x) {
  # The rank of each element, i.e. the inverse of the ordering permutation.
  o <- order.uint64(x)
  r <- integer(length(o))
  r[o] <- seq_along(o)
  return(r)
}

#' Missing values of a uint64 vector.
#'
#' A uint64 vector has no missing values, so FALSE is always returned. A uint64
#' is stored in the bit pattern of a double, which the default is.na() would
#' report as missing when all the exponent bits are set.
#' @param x uint64 vector
#' @export
is.na.uint64 <- function(x) {
  return(rep(FALSE, length(x)))
}

#' Summary group generic for uint64 vectors.
#'
#' max(), min() and range() are implemented through the uint64 ordering. Any
#' other generic of the group raises an error.
#' @param ... uint64 vectors
#' @param na.rm ignored, a uint64 vector has no missing values
#' @export
Summary.uint64 <- function(..., na.rm = FALSE) {
  x <- as.uint64(c(...))
  if (.Generic == "max") {
    return(x[order.uint64(x)[length(x)]])
  }
  if (.Generic == "min") {
    return(x[order.uint64(x)[1]])
  }
  if (.Generic == "range") {
    o <- order.uint64(x)
    return(x[c(o[1], o[length(o)])])
  }
  stop(sprintf("%s is not defined for uint64 vectors", .Generic))
}
