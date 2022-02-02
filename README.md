# PSSG

This is a very simple static site generator. In reality this does not even do the conventional
static site generation.

All this does is to allow a user to define html objects. Doing so requires the user to define a
custom object as follows:

```
  <body>
  .obj
  <Navbar />
  ....Some html
```

Here Navbar is a custom object, this is a good example which inspired the script/project. Basically
to make this work. You create a file titled `Navbar.html`, which contains proper html code for the
custom element you created. It should be exactly the same as the object you just created.
Doing this, should essentially make it so that after you run the program, it
translates and converts all those references, into the custom object you created.
