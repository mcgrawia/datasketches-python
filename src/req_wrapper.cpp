/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "py_object_lt.hpp"
#include "py_object_ostream.hpp"
#include "quantile_conditional.hpp"
#include "req_sketch.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include <vector>
#include <stdexcept>

namespace py = pybind11;

template<typename T, typename C>
void bind_req_sketch(py::module &m, const char* name) {
  using namespace datasketches;

  auto req_class = py::class_<req_sketch<T, C>>(m, name)
    .def(py::init<uint16_t, bool>(), py::arg("k")=12, py::arg("is_hra")=true)
    .def(py::init<const req_sketch<T, C>&>())
    .def("update", (void (req_sketch<T, C>::*)(const T&)) &req_sketch<T, C>::update, py::arg("item"),
        "Updates the sketch with the given value")
    .def("merge", (void (req_sketch<T, C>::*)(const req_sketch<T, C>&)) &req_sketch<T, C>::merge, py::arg("sketch"),
        "Merges the provided sketch into this one")
    .def("__str__", &req_sketch<T, C>::to_string, py::arg("print_levels")=false, py::arg("print_items")=false,
        "Produces a string summary of the sketch")
    .def("to_string", &req_sketch<T, C>::to_string, py::arg("print_levels")=false, py::arg("print_items")=false,
        "Produces a string summary of the sketch")
    .def("is_hra", &req_sketch<T, C>::is_HRA,
        "Returns True if the sketch is in High Rank Accuracy mode, otherwise False")
    .def("is_empty", &req_sketch<T, C>::is_empty,
        "Returns True if the sketch is empty, otherwise False")
    .def("get_k", &req_sketch<T, C>::get_k,
        "Returns the configured parameter k")
    .def("get_n", &req_sketch<T, C>::get_n,
        "Returns the length of the input stream")
    .def("get_num_retained", &req_sketch<T, C>::get_num_retained,
        "Returns the number of retained items (samples) in the sketch")
    .def("is_estimation_mode", &req_sketch<T, C>::is_estimation_mode,
        "Returns True if the sketch is in estimation mode, otherwise False")
    .def("get_min_value", &req_sketch<T, C>::get_min_item,
        "Returns the minimum value from the stream. If empty, req_floats_sketch returns nan; req_ints_sketch throws a RuntimeError")
    .def("get_max_value", &req_sketch<T, C>::get_max_item,
        "Returns the maximum value from the stream. If empty, req_floats_sketch returns nan; req_ints_sketch throws a RuntimeError")
    .def("get_quantile", &req_sketch<T, C>::get_quantile, py::arg("rank"), py::arg("inclusive")=false,
        "Returns an approximation to the data value "
        "associated with the given normalized rank in a hypothetical sorted "
        "version of the input stream so far.\n"
        "For req_floats_sketch: if the sketch is empty this returns nan. "
        "For req_ints_sketch: if the sketch is empty this throws a RuntimeError.")
    .def(
        "get_quantiles",
        [](const req_sketch<T, C>& sk, const std::vector<double>& ranks, bool inclusive) {
          return sk.get_quantiles(ranks.data(), ranks.size(), inclusive);
        },
        py::arg("ranks"), py::arg("inclusive")=false,
        "This returns an array that could have been generated by using get_quantile() for each "
        "normalized rank separately.\n"
        "If the sketch is empty this returns an empty vector.\n"
        "Deprecated. Will be removed in the next major version. Use get_quantile() instead."
    )
    .def("get_rank", &req_sketch<T, C>::get_rank, py::arg("value"), py::arg("inclusive")=false,
        "Returns an approximation to the normalized rank of the given value from 0 to 1, inclusive.\n"
        "The resulting approximation has a probabilistic guarantee that can be obtained from the "
        "get_normalized_rank_error(False) function.\n"
        "With the parameter inclusive=true the weight of the given value is included into the rank."
        "Otherwise the rank equals the sum of the weights of values less than the given value.\n"
        "If the sketch is empty this returns nan.")
    .def(
        "get_pmf",
        [](const req_sketch<T, C>& sk, const std::vector<T>& split_points, bool inclusive) {
          return sk.get_PMF(split_points.data(), split_points.size(), inclusive);
        },
        py::arg("split_points"), py::arg("inclusive")=false,
        "Returns an approximation to the Probability Mass Function (PMF) of the input stream "
        "given a set of split points (values).\n"
        "The resulting approximations have a probabilistic guarantee that can be obtained from the "
        "get_normalized_rank_error(True) function.\n"
        "If the sketch is empty this returns an empty vector.\n"
        "split_points is an array of m unique, monotonically increasing float values "
        "that divide the real number line into m+1 consecutive disjoint intervals.\n"
        "If the parameter inclusive=false, the definition of an 'interval' is inclusive of the left split point (or minimum value) and "
        "exclusive of the right split point, with the exception that the last interval will include "
        "the maximum value.\n"
        "If the parameter inclusive=true, the definition of an 'interval' is exclusive of the left split point (or minimum value) and "
        "inclusive of the right split point.\n"
        "It is not necessary to include either the min or max values in these split points."
    )
    .def(
        "get_cdf",
        [](const req_sketch<T, C>& sk, const std::vector<T>& split_points, bool inclusive) {
          return sk.get_CDF(split_points.data(), split_points.size(), inclusive);
        },
        py::arg("split_points"), py::arg("inclusive")=false,
        "Returns an approximation to the Cumulative Distribution Function (CDF), which is the "
        "cumulative analog of the PMF, of the input stream given a set of split points (values).\n"
        "The resulting approximations have a probabilistic guarantee that can be obtained from the "
        "get_normalized_rank_error(True) function.\n"
        "If the sketch is empty this returns an empty vector.\n"
        "split_points is an array of m unique, monotonically increasing float values "
        "that divide the real number line into m+1 consecutive disjoint intervals.\n"
        "If the parameter inclusive=false, the definition of an 'interval' is inclusive of the left split point (or minimum value) and "
        "exclusive of the right split point, with the exception that the last interval will include "
        "the maximum value.\n"
        "If the parameter inclusive=true, the definition of an 'interval' is exclusive of the left split point (or minimum value) and "
        "inclusive of the right split point.\n"
        "It is not necessary to include either the min or max values in these split points."
    )
    .def("get_rank_lower_bound", &req_sketch<T, C>::get_rank_lower_bound, py::arg("rank"), py::arg("num_std_dev"),
        "Returns an approximate lower bound on the given normalized rank.\n"
        "Normalized rank must be a value between 0.0 and 1.0 (inclusive); "
        "the number of standard deviations must be 1, 2, or 3.")
    .def("get_rank_upper_bound", &req_sketch<T, C>::get_rank_upper_bound, py::arg("rank"), py::arg("num_std_dev"),
        "Returns an approximate upper bound on the given normalized rank.\n"
        "Normalized rank must be a value between 0.0 and 1.0 (inclusive); "
        "the number of standard deviations must be 1, 2, or 3.")
    .def_static("get_RSE", &req_sketch<T, C>::get_RSE,
        py::arg("k"), py::arg("rank"), py::arg("is_hra"), py::arg("n"),
        "Returns an a priori estimate of relative standard error (RSE, expressed as a number in [0,1]). "
        "Derived from Lemma 12 in http://arxiv.org/abs/2004.01668v2, but the constant factors have been "
        "modified based on empirical measurements, for a given value of parameter k.\n"
        "Normalized rank must be a value between 0.0 and 1.0 (inclusive). If is_hra is True, uses high "
        "rank accuracy mode, else low rank accuracy. N is an estimate of the total number of points "
        "provided to the sketch.")
    .def("__iter__", [](const req_sketch<T, C>& s) { return py::make_iterator(s.begin(), s.end()); });

    add_serialization<T>(req_class);
    add_vector_update<T>(req_class);
}

void init_req(py::module &m) {
  bind_req_sketch<int, std::less<int>>(m, "req_ints_sketch");
  bind_req_sketch<float, std::less<float>>(m, "req_floats_sketch");
  bind_req_sketch<py::object, py_object_lt>(m, "req_items_sketch");
}