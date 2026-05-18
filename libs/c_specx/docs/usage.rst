Usage
=====

How to link against and call c_specx.

Linking
-------

Add ``-lc_specx`` to your link flags::

   cc myapp.c -lc_specx -o myapp

Quick example
-------------

.. code-block:: c

   #include <c_specx.h>

   int main(void) {
       c_specx_init();
       return 0;
   }
