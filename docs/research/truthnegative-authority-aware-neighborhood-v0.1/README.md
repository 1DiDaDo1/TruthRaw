# TruthNegative Authority-Aware Neighborhood v0.1

Status: EXECUTABLE N2 RESEARCH PRIMITIVE — NOT PRODUCTION-PROMOTED.

This module supplies a conservative local estimate for the bounded residual estimator.

A neighbor is rejected when it is UNKNOWN/CENSORED, crosses a censor boundary, belongs to another object, is another channel, lacks admitted variance, or differs from the center by more than two combined sigmas.

Accepted neighbors are weighted by spatial proximity, uncertainty-normalized radiometric compatibility and inverse variance. At least two compatible contributors are required.

The estimator does not alter Scientific Master/TruthNegative, creates no evidence and performs no scientific writeback. Object identity is currently an input contract; a later Deep Scene adapter must supply it rather than infer it here.
