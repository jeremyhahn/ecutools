FILE(REMOVE_RECURSE
  "CMakeFiles/hello.dir/hello.c.o"
  "hello.pdb"
  "hello"
)

# Per-language clean rules from dependency scanning.
FOREACH(lang C)
  INCLUDE(CMakeFiles/hello.dir/cmake_clean_${lang}.cmake OPTIONAL)
ENDFOREACH(lang)
