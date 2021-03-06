PCSA Estimator
==============

This is an implementation of PCSA algorithm as described in the paper
"Probalistic Counting Algorithms for Data Base Applications", published
by Flajolet and Martin in 1985. Generally it is an advanced version of
 he 'probabilistic.c' algorithm.

Contents of the extension
-------------------------
The extension provides the following elements

* pcsa_estimator data type (may be used for columns, in PL/pgSQL)

* functions to work with the pcsa_estimator data type

    * `pcsa_size(nbitmaps int, keysize int)`
    * `pcsa_init(nbitmaps int, keysize int)`

    * `pcsa_add_item(counter pcsa_estimator, item text)`
    * `pcsa_add_item(counter pcsa_estimator, item int)`

    * `pcsa_get_estimate(counter pcsa_estimator)`
    * `pcsa_reset(counter pcsa_estimator)`

    * `length(counter pcsa_estimator)`

  The purpose of the functions is quite obvious from the names,
  alternatively consult the SQL script for more details.

* aggregate functions 

    * `pcsa_distinct(text, int, int)`
    * `pcsa_distinct(text)`

    * `pcsa_distinct(int, int, int)`
    * `pcsa_distinct(int)`

  where the 1-parameter version uses 64 bitmaps and keysize 4
  as default values for the two parameters. That's quite generous
  and it may result in unnecessarily large estimators, so if you
  can work with lower precision / expect less distinct values,
  pass the parameters explicitly.


Usage
-----
Using the aggregate is quite straightforward - just use it like a
regular aggregate function

    db=# SELECT pcsa_distinct(i, 32, 3)
         FROM generate_series(1,100000) s(i);

and you can use it from a PL/pgSQL (or another PL) like this:

    DO LANGUAGE plpgsql $$
    DECLARE
        v_counter pcsa_estimator := pcsa_init(32, 3);
        v_estimate real;
    BEGIN
        PERFORM pcsa_add_item(v_counter, 1);
        PERFORM pcsa_add_item(v_counter, 2);
        PERFORM pcsa_add_item(v_counter, 3);

        SELECT pcsa_get_estimate(v_counter) INTO v_estimate;

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
so always tune the estimator to use the lowest amount of memory.
