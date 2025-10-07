// API Configuration
const API_CONFIG = {
  BASE_URL: 'http://localhost:3000/api',
  TIMEOUT: 10000
};

// API Service Class
class APIService {
  constructor() {
    this.baseURL = API_CONFIG.BASE_URL;
    this.token = localStorage.getItem('token');
  }

  // Helper method for making HTTP requests
  async request(endpoint, options = {}) {
    const url = `${this.baseURL}${endpoint}`;
    const config = {
      headers: {
        'Content-Type': 'application/json',
        ...options.headers
      },
      ...options
    };

    // Add authorization header if token exists
    if (this.token) {
      config.headers['Authorization'] = `Bearer ${this.token}`;
    }

    try {
      const response = await fetch(url, config);
      const data = await response.json();

      if (!response.ok) {
        throw new Error(data.error || 'API request failed');
      }

      return data;
    } catch (error) {
      console.error('API Error:', error);
      throw error;
    }
  }

  // Authentication methods
  async login(username, password) {
    const data = await this.request('/auth/login', {
      method: 'POST',
      body: JSON.stringify({ username, password })
    });

    if (data.success) {
      this.token = data.token;
      localStorage.setItem('token', data.token);
      localStorage.setItem('user', JSON.stringify(data.user));
    }

    return data;
  }

  logout() {
    this.token = null;
    localStorage.removeItem('token');
    localStorage.removeItem('user');
  }

  // Room methods
  async getRooms() {
    return await this.request('/rooms');
  }

  async getRoom(roomId) {
    return await this.request(`/rooms/${roomId}`);
  }

  async getRoomAvailability(roomId, date) {
    return await this.request(`/rooms/${roomId}/availability/${date}`);
  }

  // Booking methods
  async createBooking(bookingData) {
    return await this.request('/bookings', {
      method: 'POST',
      body: JSON.stringify(bookingData)
    });
  }

  async getMyBookings() {
    return await this.request('/bookings/my');
  }

  async cancelBooking(bookingId) {
    return await this.request(`/bookings/${bookingId}`, {
      method: 'DELETE'
    });
  }

  // Device control methods
  async getDeviceSettings() {
    return await this.request('/device/settings');
  }

  async controlDevice(action, timeRange) {
    return await this.request('/device/control', {
      method: 'POST',
      body: JSON.stringify({ action, timeRange })
    });
  }

  // Health check
  async healthCheck() {
    return await this.request('/health');
  }
}

// Create global API instance
const api = new APIService();

// Utility functions
const utils = {
  // Format date for API
  formatDate(date) {
    return date.toISOString().split('T')[0];
  },

  // Format time for display
  formatTime(time) {
    return time.slice(0, 5);
  },

  // Get current user
  getCurrentUser() {
    const userStr = localStorage.getItem('user');
    return userStr ? JSON.parse(userStr) : null;
  },

  // Check if user is logged in
  isLoggedIn() {
    return !!localStorage.getItem('token');
  },

  // Show notification (you can customize this)
  showNotification(message, type = 'info') {
    alert(`${type.toUpperCase()}: ${message}`);
  }
};

// Export for use in other files
if (typeof module !== 'undefined' && module.exports) {
  module.exports = { api, utils, APIService };
}