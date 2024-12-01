add_rules("mode.debug", "mode.release")

add_requires("crow")

target("cpp")
set_kind("binary")
add_files("src/*.cpp")
add_packages("crow")
set_languages("c++20")
after_build(function(target)
	os.cp("regions.json", target:targetdir())
end)
