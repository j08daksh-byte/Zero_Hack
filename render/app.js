// NovaCPP App Initialization
// Handles theme setup and dashboard clock

(function() {
  'use strict';

  function updateClock() {
    var el = document.getElementById('nova-clock');
    if (el) {
      var now = new Date();
      el.textContent = now.toLocaleTimeString();
    }
  }

  function initDashboard() {
    updateClock();
    setInterval(updateClock, 1000);

    // Add smooth scroll for anchor links
    document.querySelectorAll('a[href^="#"]').forEach(function(a) {
      a.addEventListener('click', function(e) {
        var target = document.querySelector(this.getAttribute('href'));
        if (target) {
          e.preventDefault();
          target.scrollIntoView({ behavior: 'smooth' });
        }
      });
    });
  }

  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', initDashboard);
  } else {
    initDashboard();
  }
})();
