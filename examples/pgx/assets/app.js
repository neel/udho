(() => {
  const root = document.querySelector('#project-root');
  if (!root) return;
  const id = root.dataset.projectId;
  Promise.all([
    fetch('/assets/pgx/project.html').then(response => response.text()),
    fetch(`/api/projects/${id}`).then(response => {
      if (!response.ok) throw new Error(`request failed (${response.status})`);
      return response.json();
    })
  ]).then(([fragment, payload]) => {
    root.innerHTML = fragment;
    const project = payload.project.records[0].projects;
    document.querySelector('#project-title').textContent = project.title;
    document.querySelector('#project-description').textContent = project.description;
    document.querySelector('#task-form').action = `/projects/${id}/tasks`;
    const list = document.querySelector('#project-tasks');
    payload.tasks.records.forEach(record => {
      record = record.tasks;
      const item = document.createElement('li');
      item.className = record.completed ? 'done' : 'pending';
      item.textContent = `${record.completed ? '✓' : '○'} ${record.title}`;
      list.appendChild(item);
    });
  }).catch(error => {
    root.innerHTML = `<p class="error"></p>`;
    root.firstElementChild.textContent = error.message;
  });
})();
