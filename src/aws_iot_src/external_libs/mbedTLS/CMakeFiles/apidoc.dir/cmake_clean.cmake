FILE(REMOVE_RECURSE
  "CMakeFiles/apidoc"
)

# Per-language clean rules from dependency scanning.
FOREACH(lang)
  INCLUDE(CMakeFiles/apidoc.dir/cmake_clean_${lang}.cmake OPTIONAL)
ENDFOREACH(lang)
