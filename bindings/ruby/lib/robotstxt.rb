# frozen_string_literal: true

require 'ffi'

# Ruby bindings for Google's robots.txt parser library.
#
# @example Basic usage
#   matcher = RobotsTxt::Matcher.new
#   allowed = matcher.allowed?("User-agent: *\nDisallow: /admin/", "Googlebot", "https://example.com/page")
#   puts "Access: #{allowed ? 'allowed' : 'disallowed'}"
#
module RobotsTxt
  extend FFI::Library

  # Find the library
  LIB_NAMES = %w[librobots.dylib librobots.so robots.dll].freeze
  LIB_PATHS = [
    File.expand_path('../../../_build', __dir__),
    File.expand_path('../../../build', __dir__),
    File.expand_path('../../../cmake-build', __dir__),
    '/usr/local/lib',
    '/usr/lib',
    ENV['ROBOTS_LIB_PATH']
  ].compact

  def self.find_library
    LIB_PATHS.each do |path|
      LIB_NAMES.each do |name|
        lib_path = File.join(path, name)
        return lib_path if File.exist?(lib_path)
      end
    end
    # Fallback to system library search
    'robots'
  end

  ffi_lib find_library

  # Request-rate structure
  class RequestRateStruct < FFI::Struct
    layout :requests, :int,
           :seconds, :int
  end

  # Content-signal structure (tri-state: -1=unset, 0=no, 1=yes)
  class ContentSignalStruct < FFI::Struct
    layout :ai_train, :int8,
           :ai_input, :int8,
           :search, :int8
  end

  # FFI function bindings
  attach_function :robots_matcher_create, [], :pointer
  attach_function :robots_matcher_free, [:pointer], :void
  attach_function :robots_allowed_by_robots, [
    :pointer,           # matcher
    :pointer, :size_t,  # robots_txt, robots_txt_len
    :pointer, :size_t,  # user_agent, user_agent_len
    :pointer, :size_t   # url, url_len
  ], :bool
  attach_function :robots_matching_line, [:pointer], :int
  attach_function :robots_ever_seen_specific_agent, [:pointer], :bool
  attach_function :robots_has_crawl_delay, [:pointer], :bool
  attach_function :robots_get_crawl_delay, [:pointer], :double
  attach_function :robots_has_request_rate, [:pointer], :bool
  attach_function :robots_get_request_rate, [:pointer, :pointer], :bool
  attach_function :robots_content_signal_supported, [], :bool
  attach_function :robots_has_content_signal, [:pointer], :bool
  attach_function :robots_get_content_signal, [:pointer, :pointer], :bool
  attach_function :robots_allows_ai_train, [:pointer], :bool
  attach_function :robots_allows_ai_input, [:pointer], :bool
  attach_function :robots_allows_search, [:pointer], :bool
  attach_function :robots_is_valid_user_agent, [:pointer, :size_t], :bool
  attach_function :robots_version, [], :string

  # Request rate value.
  class RequestRate
    attr_reader :requests, :seconds

    def initialize(requests, seconds)
      @requests = requests
      @seconds = seconds
    end

    # Returns requests per second.
    def requests_per_second
      return 0.0 if @seconds.zero?

      @requests.to_f / @seconds
    end

    # Returns delay between requests in seconds.
    def delay_seconds
      return 0.0 if @requests.zero?

      @seconds.to_f / @requests
    end
  end

  # Content signal values.
  class ContentSignal
    attr_reader :ai_train, :ai_input, :search

    def initialize(ai_train, ai_input, search)
      @ai_train = ai_train
      @ai_input = ai_input
      @search = search
    end

    private

    def self.tri_state(value)
      case value
      when -1 then nil
      when 0 then false
      when 1 then true
      end
    end
  end

  # Robots.txt matcher - checks if URLs are allowed for given user-agents.
  class Matcher
    # Creates a new Matcher instance.
    def initialize
      @handle = RobotsTxt.robots_matcher_create
      raise 'Failed to create RobotsMatcher' if @handle.null?

      ObjectSpace.define_finalizer(self, self.class.release(@handle))
    end

    # Release the handle when garbage collected.
    def self.release(handle)
      proc { RobotsTxt.robots_matcher_free(handle) unless handle.null? }
    end

    # Checks if a URL is allowed for a single user-agent.
    #
    # @param robots_txt [String] The robots.txt content
    # @param user_agent [String] The user-agent string to check
    # @param url [String] The URL to check
    # @return [Boolean] True if the URL is allowed
    def allowed?(robots_txt, user_agent, url)
      robots_ptr = FFI::MemoryPointer.from_string(robots_txt)
      ua_ptr = FFI::MemoryPointer.from_string(user_agent)
      url_ptr = FFI::MemoryPointer.from_string(url)

      RobotsTxt.robots_allowed_by_robots(
        @handle,
        robots_ptr, robots_txt.bytesize,
        ua_ptr, user_agent.bytesize,
        url_ptr, url.bytesize
      )
    end

    alias is_allowed allowed?

    # Returns the line number that matched, or 0 if no match.
    def matching_line
      RobotsTxt.robots_matching_line(@handle)
    end

    # Returns true if a specific user-agent block was found (not just '*').
    def ever_seen_specific_agent?
      RobotsTxt.robots_ever_seen_specific_agent(@handle)
    end

    # Returns the crawl-delay in seconds, or nil if not specified.
    def crawl_delay
      return nil unless RobotsTxt.robots_has_crawl_delay(@handle)

      RobotsTxt.robots_get_crawl_delay(@handle)
    end

    # Returns the request-rate, or nil if not specified.
    def request_rate
      rate_struct = RequestRateStruct.new
      return nil unless RobotsTxt.robots_get_request_rate(@handle, rate_struct)

      RequestRate.new(rate_struct[:requests], rate_struct[:seconds])
    end

    # Returns true if Content-Signal support is compiled in.
    def self.content_signal_supported?
      RobotsTxt.robots_content_signal_supported
    end

    # Returns the content-signal values, or nil if not specified.
    def content_signal
      return nil unless self.class.content_signal_supported?

      signal_struct = ContentSignalStruct.new
      return nil unless RobotsTxt.robots_get_content_signal(@handle, signal_struct)

      ContentSignal.new(
        ContentSignal.send(:tri_state, signal_struct[:ai_train]),
        ContentSignal.send(:tri_state, signal_struct[:ai_input]),
        ContentSignal.send(:tri_state, signal_struct[:search])
      )
    end

    # Returns true if AI training is allowed (defaults to true if not specified).
    def allows_ai_train?
      RobotsTxt.robots_allows_ai_train(@handle)
    end

    # Returns true if AI input is allowed (defaults to true if not specified).
    def allows_ai_input?
      RobotsTxt.robots_allows_ai_input(@handle)
    end

    # Returns true if search indexing is allowed (defaults to true if not specified).
    def allows_search?
      RobotsTxt.robots_allows_search(@handle)
    end
  end

  module_function

  # Returns the library version string.
  def version
    robots_version
  end

  # Checks if a user-agent string contains only valid characters [a-zA-Z_-].
  def valid_user_agent?(user_agent)
    ua_ptr = FFI::MemoryPointer.from_string(user_agent)
    robots_is_valid_user_agent(ua_ptr, user_agent.bytesize)
  end
end
