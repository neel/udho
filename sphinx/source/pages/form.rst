*****
Form
*****
Forms are accessed through the :ref:`context <udho::context>` using :cpp:func:`ctx.form() <udho::context::form>` method. 
Forms submitted in POST request can be ``url encoded`` or ``multipart``. Different form parsing techniques are used by :ref:`udho::urlencoded_form` and :ref:`udho::multipart_form`.


API
###

.. doxygenstruct:: udho::form_
   :members:
   
.. doxygenstruct:: udho::urlencoded_form
   :members:
   
.. doxygenstruct:: udho::multipart_form
   :members:
   
