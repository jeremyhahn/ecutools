FILE(REMOVE_RECURSE
  "CMakeFiles/memcheck"
)

# Per-language clean rules from dependency scanning.
FOREACH(lang)
  INCLUDE(CMakeFiles/memcheck.dir/cmake_clean_${lang}.cmake OPTIONAL)
ENDFOREACH(lang)
