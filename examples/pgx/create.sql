CREATE TABLE projects (
    id          bigserial PRIMARY KEY,
    title       varchar NOT NULL UNIQUE,
    description text NOT NULL,
    created     timestamp without time zone NOT NULL DEFAULT now()
);

CREATE TABLE tasks (
    id        bigserial PRIMARY KEY,
    project   bigint NOT NULL REFERENCES projects(id) ON DELETE CASCADE,
    title     varchar NOT NULL,
    completed boolean NOT NULL,
    created   timestamp without time zone NOT NULL DEFAULT now()
);

CREATE INDEX tasks_project_idx ON tasks(project);
CREATE INDEX tasks_project_completed_idx ON tasks(project, completed);

INSERT INTO projects(title, description)
VALUES ('Launch example', 'A small project used by the udho PostgreSQL example');

INSERT INTO tasks(project, title, completed)
SELECT id, 'Define schema', true FROM projects WHERE title = 'Launch example';

INSERT INTO tasks(project, title, completed)
SELECT id, 'Pipeline project queries', false FROM projects WHERE title = 'Launch example';

INSERT INTO tasks(project, title, completed)
SELECT id, 'Test the HTTP response', false FROM projects WHERE title = 'Launch example';
