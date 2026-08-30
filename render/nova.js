// NovaCPP Client Bridge

function updateActiveNav(path) {
  const current = path || window.location.pathname || '/';
  document.querySelectorAll('.nav-link').forEach(link => {
    const href = link.getAttribute('href');
    if (href === current || (current === '' && href === '/')) {
      link.classList.add('active');
    } else {
      link.classList.remove('active');
    }
  });
}

function setupPolling() {
  document.querySelectorAll('[nova-poll]').forEach(el => {
    const interval = parseInt(el.getAttribute('nova-poll'));
    const targetId = el.id;
    if (interval && targetId && !el.hasAttribute('nova-poll-active')) {
      el.setAttribute('nova-poll-active', 'true');
      setInterval(async () => {
        try {
          const res = await fetch('/nova/poll', {
            method: 'POST',
            headers: { 'X-Nova-Target': targetId },
            credentials: 'same-origin'
          });
          if (res.ok) {
            const html = await res.text();
            const target = document.getElementById(targetId);
            if (target && target.innerHTML !== html) target.innerHTML = html;
          }
        } catch (err) {
          console.error('[NovaCPP] Polling error:', err);
        }
      }, interval);
    }
  });
}

async function navigateTo(path) {
  try {
    const res = await fetch('/nova/navigate', {
      method: 'POST',
      headers: { 'X-Nova-Path': path },
      credentials: 'same-origin'
    });
    if (res.ok) {
      document.getElementById('root').innerHTML = await res.text();
      setupPolling();
      updateActiveNav(path);
    }
  } catch (err) {
    console.error('[NovaCPP] Navigation error:', err);
  }
}

window.addEventListener('DOMContentLoaded', () => {
  setupPolling();
  updateActiveNav();
});

window.addEventListener('popstate', () => {
  navigateTo(window.location.pathname);
  updateActiveNav(window.location.pathname);
});

document.addEventListener('click', async (e) => {
  const link = e.target.closest('[nova-link]');
  if (link) {
    e.preventDefault();
    const path = link.getAttribute('href');
    window.history.pushState({}, '', path);
    await navigateTo(path);
    return;
  }

  const btn = e.target.closest('[nova-click]');
  if (btn) {
    const action = btn.getAttribute('nova-click');
    const targetId = btn.getAttribute('nova-target');
    const headers = {};
    if (targetId) headers['X-Nova-Target'] = targetId;

    try {
      const res = await fetch('/nova/action/' + action, {
        method: 'POST',
        headers,
        credentials: 'same-origin'
      });
      if (res.ok) {
        const html = await res.text();
        if (targetId) {
          const target = document.getElementById(targetId);
          if (target) target.innerHTML = html;
        } else {
          document.getElementById('root').innerHTML = html;
        }
        setupPolling();
        updateActiveNav();
      }
    } catch (err) {
      console.error('[NovaCPP] Action error:', err);
    }
  }
});
