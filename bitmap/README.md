Bitmap Distinct Estimator
=========================

This is an implementation of self-learning bitmap, as described in the
paper "Distinct Counting with a Self-Learning Bitmap" (by Aiyou Chen
and Jin Cao, published in 2009).

Contents of the extension
-------------------------
The extension provides the following elements

* bitmap_estimator data type (may be used for columns, in PL/pgSQL)

* functions to work with the bitmap_estimator data type

    * `bitmap_size(real error, item_size int)`
    * `bitmap_init(real error, item_size int)`

    * `bitmap_add_item(bitmap_estimator counter, item text)`
    * `bitmap_add_item(bitmap_estimator counter, item int)`

    * `bitmap_get_estimate(bitmap_estimator counter)`
    * `bitmap_get_error(bitmap_estimator counter)`
    * `bitmap_get_ndistinct(bitmap_estimator counter)`

    * `bitmap_reset(bitmap_estimator counter)`

    * `length(bitmap_estimator counter)`

  The purpose of the functions is quite obvious from the names,
  alternatively consult the SQL script for more details.

* aggregate functions 

    * `bitmap_distinct(text, real, int)
    * `bitmap_distinct(text)

    * `bitmap_distinct(int, real, int)
    * `bitmap_distinct(int)

  where the 1-parameter version uses 0.025 (2.5%) and 1.000.000
  as default values for the two parameters. That's quite generous
  and it may result in unnecessarily large estimators, so if you
  can work with lower precision / expect less distinct values,
  pass the parameters explicitly.


Usage
-----
Using the aggregate is quite straightforward - just use it like a
regular aggregate function

    db=# SELECT bitmap_distinct(i, 0.01, 100000)
         FROM generate_series(1,100000) s(i);

and you can use it from a PL/pgSQL (or another PL) like this:

    DO LANGUAGE plpgsql $$
    DECLARE
        v_counter bitmap_estimator := bitmap_init(0.01,10000);
        v_estimate real;
    BEGIN
        PERFORM bitmap_add_item(v_counter, 1);
        PERFORM bitmap_add_item(v_counter, 2);
        PERFORM bitmap_add_item(v_counter, 3);

        SELECT bitmap_get_estimate(v_counter) INTO v_estimate;

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
