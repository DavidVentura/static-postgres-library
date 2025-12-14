-- example_static extension SQL script version 1.0

-- Function: add_one
-- Adds 1 to the input integer
CREATE FUNCTION add_one(integer) RETURNS integer
AS 'example_static', 'add_one'
LANGUAGE C STRICT IMMUTABLE;

COMMENT ON FUNCTION add_one(integer) IS 'Adds 1 to the input value';

-- Function: hello_world
-- Returns a greeting string
CREATE FUNCTION hello_world() RETURNS text
AS 'example_static', 'hello_world'
LANGUAGE C STRICT IMMUTABLE;

COMMENT ON FUNCTION hello_world() IS 'Returns a greeting from the static extension';
