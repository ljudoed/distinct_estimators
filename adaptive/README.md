Adaptive Distinct Estimator
===========================

This is an implementation of Adaptive Sampling algorithm presented in
paper "On Adaptive Sampling" pub. in 1990 (written by P. Flajolet).

Contents of the extension
-------------------------
The extension provides the following elements

* adaptive_estimator data type (may be used for columns, in PL/pgSQL)

* functions to work with the adaptive_estimator data type

    * `adaptive_size(error real, ndistinct int)`
    * `adaptive_init(error real, ndistinct int)`

    * `adaptive_add_item(adaptive_estimator counter, item text)`
    * `adaptive_add_item(adaptive_estimator counter, item int)`

    * `adaptive_get_estimate(adaptive_estimator counter)`
    * `adaptive_get_error(adaptive_estimator counter)`
    * `adaptive_get_ndistinct(adaptive_estimator counter)`
    * `adaptive_get_item_size(adaptive_estimator counter)`

    * `adaptive_reset(adaptive_estimator counter)`

    * `adaptive_merge(adaptive_estimator c1, adaptive_estimator c2)`

    * `length(adaptive_estimator counter)`

  The purpose of the functions is quite obvious from the names,
  alternatively consult the SQL script for more details.

* aggregate functions 

    * `adaptive_distinct(text, real, int)`
    * `adaptive_distinct(text)`
    * `adaptive_distinct(int, real, int)`
    * `adaptive_distinct(int)`

  where the 1-parameter version uses 0.025 (2.5%) and 1.000.000
  as default values for the two parameters. That's quite generous
  and it may result in unnecessarily large estimators, so if you
  can work with lower precision / expect less distinct values,
  pass the parameters explicitly.


Usage
-----
Using the aggregate is quite straightforward - just use it like a
regular aggregate function

    db=# SELECT adaptive_distinct(i, 0.01, 100000)
         FROM generate_series(1,100000) s(i);

and you can use it from a PL/pgSQL (or another PL) like this:

    DO LANGUAGE plpgsql $$
    DECLARE
        v_counter adaptive_estimator := adaptive_init(0.01,10000);
        v_estimate real;
    BEGIN
        PERFORM adaptive_add_item(v_counter, 1);
        PERFORM adaptive_add_item(v_counter, 2);
        PERFORM adaptive_add_item(v_counter, 3);

        SELECT adaptive_get_estimate(v_counter) INTO v_estimate;

        RAISE NOTICE 'estimate = %',v_estimate;
    END$$;

And this can be done from a trigger (updating an estimate stored
in a table).


Problems
--------
Be careful about the implementation, as the estimators may easily
occupy several kilobytes (depends on the precision etc.). Keep in
mind that the PostgreSQL MVCC works so that it creates a copy of
the row on update, an that may easily lead to bloat. So group the
updates or something like that.

This is of course made worse by using unnecessarily large estimators,
so always tune the estimator to use the lowest acceptable precision
and lowest expected number of distinct elements (because that's what
increases the estimator size).
